#include <iostream>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include "tengine_c_api.h"
#include "common.hpp"
#include "common_util.hpp"

#define DEFAULT_PROTO "FSRCNN-s_net.prototxt"
#define DEFAULT_MODEL "TCL_s_x2_denoise_v4.caffemodel"
const char MODEL_NAME[] = "model";
const int IMAGE_HEIGHT = 1018;
const int IMAGE_WIDTH = 766;
int IMAGE_SIZE = IMAGE_HEIGHT * IMAGE_WIDTH;

struct Config {
    Config(): proto_file(""), model_file(""), cpus(""), stop(false) {}

    Config(const int argc, char* argv[]): Config() {
        const std::string root_path = get_root_path();

        int res;
        while ((res=getopt(argc,argv,"p:m:c:h"))!= -1) {
            switch(res) {
                case 'p':
                    proto_file=optarg;
                    break;
                case 'm':
                    model_file=optarg;
                    break;
                case 'c':
                    cpus = optarg;
                    break;
                case 'h':
                    std::cout << "[Usage]: " << argv[0] << " [-h]\n"
                              << "   [-p proto_file] [-m model_file] [-c comma separated cpu list]\n";
                    stop = true;
                default:
                    break;
            }
        }

        if(proto_file.empty()) {
            proto_file = root_path + DEFAULT_PROTO;

        }
        if(model_file.empty()) {
            model_file = root_path + DEFAULT_MODEL;
        }
    }

    std::string proto_file;
    std::string model_file;
    std::string cpus;
    bool stop;
};

void get_input_data(float *input_data, int img_h, int img_w) {
    for (int h = 0; h < img_h; h++) {
        for (int w = 0; w < img_w; w++) {
            input_data[h * img_w + w] = 1 / (1 + h + w);
        }
    }
}

graph_t init(const Config &config) {
    init_tengine_library();

    if (config.cpus.length()) {
        char *cpus_copy(new char[config.cpus.length() + 1]);
        strcpy(cpus_copy, config.cpus.c_str());
        TEngine::set_cpu_list(cpus_copy);
    }

    if (load_model(MODEL_NAME, "caffe", config.proto_file.c_str(), config.model_file.c_str()) < 0)
        throw "loading model error";

    graph_t graph = create_runtime_graph("graph", MODEL_NAME, NULL);
    if (!check_graph_valid(graph)) {
        throw "invalid graph error";
    }

    float *input_data = (float *)malloc(sizeof(float) * IMAGE_SIZE);

    int node_idx=0;
    int tensor_idx=0;
    tensor_t input_tensor = get_graph_input_tensor(graph, node_idx, tensor_idx);

    if (!check_tensor_valid(input_tensor)) {
        printf("Get input node failed : node_idx: %d, tensor_idx: %d\n",node_idx,tensor_idx);
        throw "invalid input error";
    }

    int dims[] = {1, 1, IMAGE_HEIGHT, IMAGE_WIDTH};
    set_tensor_shape(input_tensor, dims, 4);
    int error = prerun_graph(graph);
    if (error) {
        std::cout << "error during prerun: " << error << std::endl;
        throw "prerun error";
    }
    get_input_data(input_data, IMAGE_HEIGHT,  IMAGE_WIDTH);

    set_tensor_buffer(input_tensor, input_data, IMAGE_SIZE * sizeof(float));
    return graph;
}


static inline int get_env_variable_int(const char *variable_name, const int default_value) {
    const char *value_str = std::getenv(variable_name);
    return (value_str) ? std::strtoul(value_str, NULL, 0) : default_value;
}

int main(int argc, char *argv[]) {

    Config config(argc, argv);
    if (config.stop) {
        return 0;
    }

    graph_t graph = init(config);

    int repeat_count(get_env_variable_int("REPEAT_COUNT", 1));
    int threads_number(get_env_variable_int("OPENBLAS_NUM_THREADS", -1));
    int sleep_seconds(get_env_variable_int("SLEEP_TIME", 0));

    struct timeval t0, t1;

    for (int i = 0; i < repeat_count; i++) {
        gettimeofday(&t0, NULL);
        run_graph(graph, 1);
        gettimeofday(&t1, NULL);

        float mytime = (float)((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec)) / 1000;
        if (config.cpus.size()) {
            printf("%f\t%d\t%s", mytime, threads_number, config.cpus.c_str());
        } else {
            printf("%f\t%d", mytime, threads_number);
        }
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
    }

    return 0;
}
