#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <cstddef>
#include <string>
#include <vector>

struct EngineSample {
    double timeSeconds;
    double engineTorqueNm;
    double engineRpm;
};

class CsvParser {
public:
    bool load(const std::string& filePath, std::string* errorMessage = 0);
    const std::vector<EngineSample>& getSamples() const;

private:
    static const double MAX_ENGINE_TORQUE_NM;
    static const double MAX_ENGINE_RPM;

    std::vector<EngineSample> samples_;

    static bool hasRequiredColumns(const std::vector<std::string>& columnNames);
    bool validateAndStoreSamples(const std::vector<double>& times,
                                 const std::vector<double>& torques,
                                 const std::vector<double>& rpms,
                                 std::string* errorMessage);
};

#endif