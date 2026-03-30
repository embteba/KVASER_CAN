#include "csv_parser.h"

#include "third_party/rapidcsv.h"

#include <algorithm>
#include <exception>
#include <sstream>

const double CsvParser::MAX_ENGINE_TORQUE_NM = 6553.5;
const double CsvParser::MAX_ENGINE_RPM = 16383.75;

namespace {
std::string buildRowError(size_t rowNumber, const std::string& message) {
    std::ostringstream stream;
    stream << "CSV validation error at row " << rowNumber << ": " << message;
    return stream.str();
}
}

bool CsvParser::load(const std::string& filePath, std::string* errorMessage) {
    samples_.clear();

    try {
        rapidcsv::Document document(filePath);
        const std::vector<std::string> columnNames = document.GetColumnNames();
        if (!hasRequiredColumns(columnNames)) {
            if (errorMessage != 0) {
                std::ostringstream stream;
                stream << "CSV header mismatch. Expected: Time, ENGINE_Torque, ENGINE_RPM. Found: ";
                for (size_t i = 0; i < columnNames.size(); ++i) {
                    if (i > 0) stream << ", ";
                    stream << columnNames[i];
                }
                *errorMessage = stream.str();
            }
            return false;
        }

        const std::vector<double> times = document.GetColumn<double>("Time");
        const std::vector<double> torques = document.GetColumn<double>("ENGINE_Torque");
        const std::vector<double> rpms = document.GetColumn<double>("ENGINE_RPM");

        return validateAndStoreSamples(times, torques, rpms, errorMessage);
    }
    catch (const std::exception& exception) {
        if (errorMessage != 0) {
            *errorMessage = "Failed to read CSV file: " + filePath + " (" + exception.what() + ")";
        }
        return false;
    }
}

const std::vector<EngineSample>& CsvParser::getSamples() const {
    return samples_;
}

bool CsvParser::hasRequiredColumns(const std::vector<std::string>& columnNames) {
    return std::find(columnNames.begin(), columnNames.end(), "Time") != columnNames.end()
        && std::find(columnNames.begin(), columnNames.end(), "ENGINE_Torque") != columnNames.end()
        && std::find(columnNames.begin(), columnNames.end(), "ENGINE_RPM") != columnNames.end();
}

bool CsvParser::validateAndStoreSamples(const std::vector<double>& times,
                                        const std::vector<double>& torques,
                                        const std::vector<double>& rpms,
                                        std::string* errorMessage) {
    if (times.size() != torques.size() || times.size() != rpms.size()) {
        if (errorMessage != 0) {
            *errorMessage = "CSV columns have inconsistent lengths";
        }
        return false;
    }

    if (times.empty()) {
        if (errorMessage != 0) {
            *errorMessage = "CSV file contains no sample rows";
        }
        return false;
    }

    samples_.reserve(times.size());
    double previousTimeSeconds = -1.0;

    for (size_t index = 0; index < times.size(); ++index) {
        EngineSample sample;
        sample.timeSeconds = times[index];
        sample.engineTorqueNm = torques[index];
        sample.engineRpm = rpms[index];

        const size_t rowNumber = index + 2;
        if (sample.timeSeconds < 0.0) {
            if (errorMessage != 0) {
                *errorMessage = buildRowError(rowNumber, "Time must be non-negative");
            }
            samples_.clear();
            return false;
        }
        if (previousTimeSeconds >= 0.0 && sample.timeSeconds < previousTimeSeconds) {
            if (errorMessage != 0) {
                *errorMessage = buildRowError(rowNumber, "Time must be monotonically increasing");
            }
            samples_.clear();
            return false;
        }
        if (sample.engineTorqueNm < 0.0 || sample.engineTorqueNm > MAX_ENGINE_TORQUE_NM) {
            if (errorMessage != 0) {
                std::ostringstream stream;
                stream << "ENGINE_Torque " << sample.engineTorqueNm << " Nm is outside DBC range [0, "
                       << MAX_ENGINE_TORQUE_NM << "]";
                *errorMessage = buildRowError(rowNumber, stream.str());
            }
            samples_.clear();
            return false;
        }
        if (sample.engineRpm < 0.0 || sample.engineRpm > MAX_ENGINE_RPM) {
            if (errorMessage != 0) {
                std::ostringstream stream;
                stream << "ENGINE_RPM " << sample.engineRpm << " rpm is outside DBC range [0, "
                       << MAX_ENGINE_RPM << "]";
                *errorMessage = buildRowError(rowNumber, stream.str());
            }
            samples_.clear();
            return false;
        }

        samples_.push_back(sample);
        previousTimeSeconds = sample.timeSeconds;
    }

    return true;
}