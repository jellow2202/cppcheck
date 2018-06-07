/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2018 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "teamcityoutput.h"

#include "check.h"
#include "preprocessor.h"
#include "settings.h"

#include <iostream>
#include <sstream>
#include "path.h"

std::ostream& TeamCityOutput::output = std::cout;

TeamCityOutput::TeamCityOutput(const Settings *settings)
    : _settings(settings)
{
}

void TeamCityOutput::reportOut(const std::string &outmsg)
{
    output << formatServiceMessage("message", { { "text", outmsg } }) << '\n';
}

void TeamCityOutput::reportProgress(const std::string &filename, const char stage[], const std::size_t /*value*/)
{
    // Only report when a new stage or file is reached
    if (_latestProgressFile == filename && _latestProgressStage == stage)
        return;
    _latestProgressFile = filename;
    _latestProgressStage = stage;
    std::stringstream ss;
    ss << "inspecting '" << filename << "' stage: " << stage;
    output << formatServiceMessage("progressMessage", ss.str()) << '\n';
}

void TeamCityOutput::reportErr(const ErrorLogger::ErrorMessage &msg)
{
    // Alert only about unique errors
    const std::string serializedMsg = msg.serialize();
    if (!_errorList.insert(serializedMsg).second)
        return;

    std::map<std::string, std::string> values;
    values["typeId"] = msg._id;
    values["message"] = _settings->verbose ? msg.verboseMessage() : msg.shortMessage();
    if (msg._callStack.empty()) {
        values["file"] = msg.file0;
    } else {
        const ErrorMessage::FileLocation& stackEntry = msg._callStack.front();
        values["file"] = stackEntry.getfile();
        values["line"] = std::to_string(stackEntry.line);
        values["column"] = std::to_string(stackEntry.col);
    }
    if (msg._cwe.id) {
        values["cwe"] = std::to_string(msg._cwe.id);
    }
    if (msg._inconclusive) {
        values["inconclusive"] = "true";
    }
    if (values["file"].empty()) {
        // Set a fake filename for cppcheck internal errors
        values["file"] = "<cppcheck>";
    } else {
        values["file"] = Path::fromNativeSeparators(values["file"]);
        // Some checks return absolute paths. TC needs them relative
        if (Path::isAbsolute(values["file"])) {
            values["file"] = Path::getRelativePath(values["file"], _settings->basePaths);
        }
        // Prefix . directory to prevent empty folder labels in TC
        values["file"] = "./" + values["file"];
    }

    switch (msg._severity)  {
        case Severity::error:
            values["SEVERITY"] = "ERROR";
            break;
        case Severity::warning:
            values["SEVERITY"] = "WARNING";
            break;
        case Severity::information:
        case Severity::debug:
        case Severity::style:
            values["SEVERITY"] = "INFO";
            break;
        case Severity::performance:
        case Severity::portability:
            values["SEVERITY"] = "WEAK WARNING";
            break;
        case Severity::none:
        default:
            break;
    }
    output << formatServiceMessage("inspection", values) << '\n';
}

void TeamCityOutput::reportInspectionTypes() const
{
    // anonymous inline ErrorLogger class to format TeamCity inspectionType messages
    class : public ErrorLogger {
    public:
        void reportOut(const std::string &) override { }
        void reportErr(const ErrorLogger::ErrorMessage &msg) override {
            output << formatServiceMessage("inspectionType", {
                { "id", msg._id },
                { "name", msg._id },
                { "description", verbose ? msg.verboseMessage() : msg.shortMessage() },
                { "category", "cppcheck " + Severity::toString(msg._severity) }
            }) << '\n';
        }
        bool verbose = false;
    } inspectionTypeLogger;

    inspectionTypeLogger.verbose = _settings->verbose;

    // call all "getErrorMessages" in all registered Check classes
    for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it)
        (*it)->getErrorMessages(&inspectionTypeLogger, _settings);

    Preprocessor::getErrorMessages(&inspectionTypeLogger, _settings);
}

std::string TeamCityOutput::serviceMessageEscape(const std::string& str)
{
    std::string result = str;
    std::string::size_type pos = 0;
    // lopp over each occurance of one of the following characters in result:
    //  '  (single-quote)
    //  \n (new line)
    //  \r (carriage return)
    //  [] (open/closing bracket)
    //  |  (pipe)
    // prefix every occurance with TeamCitys escape character | (pipe)
    // and replace \n and \r with their corresponding letters
    while (pos < result.length() && (pos = result.find_first_of("'\n\r[]|", pos)) != std::string::npos) {
        if (result[pos] == '\n') {
            result[pos] = 'n';
        } else if (result[pos] == '\r') {
            result[pos] = 'r';
        }
        result.insert(pos, "|");
        pos+=2;
    }
    return result;
}

std::string TeamCityOutput::formatServiceMessage(const std::string& messageName,
        const std::map<std::string, std::string>& values)
{
    std::stringstream ss;
    ss << "##teamcity[" << messageName;
    for (const auto& value : values)
        ss << ' ' << value.first << "='" << serviceMessageEscape(value.second) << '\'';
    ss << ']';
    return ss.str();
}

std::string TeamCityOutput::formatServiceMessage(const std::string& messageName, const std::string& value)
{
    std::stringstream ss;
    ss << "##teamcity[" << messageName << " '" << serviceMessageEscape(value) << "']";
    return ss.str();
}
