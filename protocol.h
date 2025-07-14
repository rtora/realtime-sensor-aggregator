#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <sstream>
#include <vector>
#include <map>

struct SensorData {
    double latitude = 34.0522;
    double longitude = -118.2437;
    double temperature = 25.0;

    std::string serialize() const {
        std::stringstream ss;
        ss << "LAT:" << latitude << ";LON:" << longitude << ";TEMP:" << temperature << ";";
        return ss.str();
    }

    // New Function: Parses a string back into this struct
    static SensorData deserialize(const std::string& str) {
        SensorData data;
        std::stringstream ss(str);
        std::string segment;
        std::map<std::string, double> values;

        while(std::getline(ss, segment, ';')) {
            size_t pos = segment.find(':');
            if (pos != std::string::npos) {
                std::string key = segment.substr(0, pos);
                double value = std::stod(segment.substr(pos + 1));
                values[key] = value;
            }
        }

        if (values.count("LAT")) data.latitude = values["LAT"];
        if (values.count("LON")) data.longitude = values["LON"];
        if (values.count("TEMP")) data.temperature = values["TEMP"];
        
        return data;
    }
};

#endif // PROTOCOL_H