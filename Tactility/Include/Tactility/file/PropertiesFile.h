#pragma once

#include <functional>
#include <map>
#include <string>

/**
 * @note The functionality below safely acquires and releases any SD card device locks. Manual locking isn't needed.
 */
namespace tt::file {

/**
 * Load a properties file into a map
 * @param[in] filePath the file to load
 * @param[out] properties the resulting properties
 * @return true when the properties file was opened successfully
 */
bool loadPropertiesFile(const std::string& filePath, std::map<std::string, std::string>& properties);

/**
 * Load a properties file and report key-values to a function
 * @param[in] filePath the file to load
 * @param[in] callback the callback function that receives the key-values
 * @return true when the properties file was opened successfully
 */
bool loadPropertiesFile(const std::string& filePath, std::function<void(const std::string& key, const std::string& value)> callback);

/**
 * Save properties to a file
 * @param[in] filePath the file to save to
 * @param[in] properties the properties to save
 * @return true when the data was written to the file succesfully
 */
bool savePropertiesFile(const std::string& filePath, const std::map<std::string, std::string>& properties);

}
