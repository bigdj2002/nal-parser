#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <getopt.h>

#include "nal_parse.h"

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " --file_path <NAL stream file> --codec_type <h264|hevc|vvc>" << std::endl;
        return 1;
    }

    const char *filePath = nullptr;
    const char *codecTypeStr = nullptr;

    static struct option long_options[] = {
        {"file_path", required_argument, 0, 'f'},
        {"codec_type", required_argument, 0, 'c'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "f:c:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'f':
            filePath = optarg;
            break;
        case 'c':
            codecTypeStr = optarg;
            break;
        case '?':
            std::cerr << "Unknown option." << std::endl;
            return 1;
        default:
            std::cerr << "Usage: " << argv[0] << " --file_path <NAL stream file> --codec_type <h264|hevc|vvc>" << std::endl;
            return 1;
        }
    }

    if (!filePath || !codecTypeStr)
    {
        std::cerr << "Both --file_path and --codec_type are required." << std::endl;
        return 1;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return 1;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> nalData(fileSize);
    file.read(reinterpret_cast<char *>(nalData.data()), fileSize);

    NALParse *nalParseHandler = new NALParse();
    int nextNalPos = 0;
    nal_info *&nal = nalParseHandler->nal;
    uint8_t *data = nalData.data();
    int dataSize = fileSize;
    std::string codecStr(codecTypeStr);
    int cIdx = codecStr == "h264" ? 1 : codecStr == "hevc" ? 2
                                    : codecStr == "vvc"    ? 3
                                                           : -1;
    videoCodecType codecType = static_cast<videoCodecType>(cIdx);

    while (nextNalPos < dataSize && 0 < cIdx)
    {
        nalParseHandler->nal_parse(data, codecType, nextNalPos, dataSize, parsingLevel::PARSING_FULL);
    }

    file.close();
    delete nalParseHandler;

    return 0;
}
