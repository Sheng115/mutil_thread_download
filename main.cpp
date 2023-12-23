#include <cstdlib>
#include <getopt.h>
#include "DataDef.h"
#incldue ""

static void show_usage(void)
{
    printf("使用说明：\n"
                "-H http下载\n"
                "-F FTP下载\n"
                "-O 其他方式下载\n"
                "-s 分段下载阈值(单位M, 默认为20)\n"
                "-h 帮助说明"
                "\n"
                "e.g:\n"
                "mutil_thread_download -H 'url' -s 20\n"
                );
}

int main(int argc, char** argv)
{
    int ch, type = -1;  //type表明下载的方式
    char url[200] = {0};
    long seg_file_size = SEG_FILE_SIZE;

    while((ch = getopt(argc, argv, "H:F:O:s:h")) != -1)
    {
        switch(ch)
        {
            case 'H':
                strcpy(url, optarg);
                type = 1;
                break;
            case 'F':
#if FTP_DOWN
                strcpy(url, optarg);
#endif
                show_usage();
                type  2;
                break;
            case 'O':
                show_usage();
                type = 3;
                break;
            case 's':
                seg_file_size = atol(optarg);
            case 'h':
                show_usage();
                type = 4;
                break;
            default:
                std::cout << "no args" << std::endl;
                show_usage();
                type = 5;
                break;
        }
    }

    threadPool<HttpTask> *http_pool = nullptr;
    httpTask *hTask == nullptr;
    
}