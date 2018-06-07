/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2017 Cppcheck team.
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

#ifndef TEAMCITYOUTPUT_H
#define TEAMCITYOUTPUT_H

#include "errorlogger.h"
#include <string>
#include <map>
#include <set>

class Settings;

/**
 * An \ref ErrorLogger that outputs TeamCity service messages.
 *
 * TeamCity service messages allow for oob integration of tools into TeamCity.
 * This \ref ErrorLogger will be invoked by \ref CppCheckExecutor when the
 * \c --teamcity commandline-argument is given.
 *
 * @see https://confluence.jetbrains.com/display/TCD10/Build+Script+Interaction+with+TeamCity
 */
class TeamCityOutput : public ErrorLogger {
public:
    /**
     * Constructor
     */
    TeamCityOutput(const Settings *settings);

    /**
     * Destructor
     */
    ~TeamCityOutput() override = default;

    /**
     * @copydoc ErrorLogger::reportOut
     */
    void reportOut(const std::string &outmsg) override;

    /**
     * @copydoc ErrorLogger::reportErr
     */
    void reportErr(const ErrorLogger::ErrorMessage &msg) override;

    /**
     * @copydoc ErrorLogger::reportProgress
     */
    void reportProgress(const std::string &filename, const char stage[], const std::size_t value) override;

    /**
     * Reports the list of all possible errors to TeamCity
     */
    void reportInspectionTypes() const;

    /**
    * Output stream used by TeamCityOutput (default: std::cout)
    * @note should only be changed for testing purposes
    */
    static std::ostream& output;

private:
    /**
     * A pointer to the parent \ref CppCheck instance
     */
    const Settings *_settings;

    /**
     * Last progress report filename
     */
    std::string _latestProgressFile;

    /**
    * Last progress report stage
    */
    std::string _latestProgressStage;

    /**
     * Used to filter out duplicate error messages.
     */
    std::set<std::string> _errorList;

    /**
     * String escape for TeamCity service message values
     *
     * @param str the string to esapce
     * @return the escaped string
     * @see https://confluence.jetbrains.com/display/TCD10/Build+Script+Interaction+with+TeamCity#BuildScriptInteractionwithTeamCity-Escapedvalues
     */
    static std::string serviceMessageEscape(const std::string& str);

    /**
     * Formats a multi-attribute TeamCity service message for console output
     *
     * @param messageName The name of the message
     * @param values A map of key-value pairs used as message values
     * @return The formatted service message string to be send to the console
     * @see https://confluence.jetbrains.com/display/TCD10/Build+Script+Interaction+with+TeamCity#BuildScriptInteractionwithTeamCity-Servicemessagesformats
     */
    static std::string formatServiceMessage(const std::string& messageName, const std::map<std::string, std::string>& values);

    /**
    * Formats a single-attribute TeamCity service message for console output
    *
    * @param messageName The name of the message
    * @param value the message text
    * @return The formatted service message string to be send to the console
    * @see https://confluence.jetbrains.com/display/TCD10/Build+Script+Interaction+with+TeamCity#BuildScriptInteractionwithTeamCity-Servicemessagesformats
    */
    static std::string formatServiceMessage(const std::string& messageName, const std::string& value);
};

#endif // TEAMCITYOUTPUT_H
