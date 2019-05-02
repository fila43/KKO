#include "kko.h"


int main(int argc, char * argv[])
{

    const char* const short_opts = "cdh:mo:i:w";
    const option long_opts[] = {
            {"c", no_argument, nullptr, 'c'},
            {"d", no_argument, nullptr, 'd'},
            {"h",required_argument, nullptr, 'h'},
            {"m", no_argument, nullptr, 'm'},
            {"o", required_argument, nullptr, 'o'},
            {"i", required_argument, nullptr, 'i'},
            {"w", optional_argument, nullptr, 'w'},

            {nullptr, no_argument, nullptr, 0}
    };

    int mode =-1;      //compress/decompress
    int method = -1;    //static/dynamic
    bool model = false;     //model
    std::string inputFile,outputFile;
    unsigned int width;

    while (true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt)
            break;

        switch (opt)
        {
            case 'c':
                mode = 1;
                break;

            case 'd':
                mode = 0;
                break;

            case 'h':

                if (std::string(optarg) == "static") {
                    method = 1;
                    break;
                }
                if (std::string(optarg) == "adaptive") {
                    method = 0;
                    break;
                }

                std::cerr << "Chyba"<< std::endl;
                break;

            case 'm':
                model = true;
                break;

            case 'i':
                inputFile = std::string(optarg);
                break;
            case 'o':
                outputFile = std::string(optarg);
                break;
            case 'w':
               // width = std::stoi(optarg);
                break;
            case '?': // Unrecognized option
            default:
                for (int i = 0; i< argc;i++){
                    std::cerr<<argv[i]<<std::endl;
                }

                break;
        }
    }




    /*
     * Load file
     */

    std::ifstream input( inputFile, std::ios::binary );
    std::vector<uint8_t > buffer(std::istreambuf_iterator<char>(input), {});

    if (buffer.empty()) {
        std::cerr<<"cant open file!!\n";
        return 1;
    }

    HTree huff = HTree(buffer);


    if (mode == 1){                     //compress
     //   std::cout<<inputFile;
        if (model){                         //model
            huff.applyModel();
        }
        if (method == 1){                       //static

            huff.runStaticCompress(outputFile);
        } else if (method == 0){                //adaptive
                if (model){
                    huff.runAdaptiveCompress(outputFile,huff.applyModel(buffer));

                } else{
                    huff.runAdaptiveCompress(outputFile,buffer);

                }
        } else{                                 //arg error

        }
    } else if(mode == 0){                          //decompress
        if (model)
            huff.setModel(true);
        if (method == 1){                       //static

            huff.runStaticDeCompress(outputFile);

        } else if (method == 0){                //adaptive

            huff.runAdaptiveDeCompress(outputFile);

        } else{                                 //arg error
            std::cerr<<"arg error\n";
        }
    } else{                 //error in arguments
        std::cerr<<"arg error\n";
    }

    return 1;


}



