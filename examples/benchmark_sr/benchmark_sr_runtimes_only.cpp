#include <iostream>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include "tengine_c_api.h"
#include "common.hpp"
#include "common_util.hpp"

static const std::string DEFAULT_PROTO("FSRCNN-s_net.prototxt");
static const std::string DEFAULT_MODEL("TCL_s_x2_denoise_v4.caffemodel");
static const std::string MODEL_NAME("model");
static const int IMAGE_HEIGHT = 1018;
static const int IMAGE_WIDTH = 766;
static int IMAGE_SIZE = IMAGE_HEIGHT * IMAGE_WIDTH;

struct Config {
    Config() : proto_file(""), model_file(""), cpus(""), stop(false) {}

    Config(const int argc, char *argv[]) : Config() {
        const std::string root_path = get_root_path();

        int res;
        while ((res = getopt(argc, argv, "p:m:c:h")) != -1) {
            switch (res) {
                case 'p':
                    proto_file = optarg;
                    break;
                case 'm':
                    model_file = optarg;
                    break;
                case 'c':
                    cpus = optarg;
                    break;
                case 'h':
                    printf("[Usage]: %s [-h] [-p proto_file] [-m model_file - should have the same input size like the "
                           "original SR model] [-c comma separated cpu list]\n", argv[0]);
                    stop = true;
                    break;
                default:
                    break;
            }
        }

        if (proto_file.empty()) {
            proto_file = root_path + DEFAULT_PROTO;
        }
        if (model_file.empty()) {
            model_file = root_path + DEFAULT_MODEL;
        }
    }

    std::string proto_file;
    std::string model_file;
    std::string cpus;
    bool stop;
};

static void remove_model() {
    remove_model(MODEL_NAME.c_str());
}

static void delete_graph(graph_t graph) {
    postrun_graph(graph);
    destroy_runtime_graph(graph);
}

struct Environment {
    Environment(graph_t graph, float input_data[]) : input_data(input_data), graph(graph) {}

    ~Environment() {
        delete_graph(graph);
        remove_model();
    }

    std::shared_ptr<float> input_data;
    graph_t graph;
};

static void get_input_data(float *input_data, int img_h, int img_w) {
    for (int h = 0; h < img_h; h++) {
        for (int w = 0; w < img_w; w++) {
            input_data[h * img_w + w] = 1 / (1 + h + w);
        }
    }
}

Environment init(const Config &config) {
    int error = init_tengine_library();
    if (error) {
        printf("error during init_tengine_library: %d\n", error);
        throw "init_tengine_library error";
    }

    if (config.cpus.length()) {
        std::unique_ptr<char[]> cpus_copy(new char[config.cpus.length() + 1]);
        // We need to copy the argument because function modifies underlying char array
        strcpy(cpus_copy.get(), config.cpus.c_str());
        TEngine::set_cpu_list(cpus_copy.get());
    }

    if (load_model(MODEL_NAME.c_str(), "caffe", config.proto_file.c_str(), config.model_file.c_str()) < 0) {
        remove_model();
        throw "loading model error";
    }

    graph_t graph(create_runtime_graph("graph", MODEL_NAME.c_str(), nullptr));
    if (!check_graph_valid(graph)) {
        delete_graph(graph);
        remove_model();
        throw "invalid graph error";
    }

    Environment environment(graph, new float[IMAGE_SIZE]);

    int node_idx = 0;
    int tensor_idx = 0;
    tensor_t input_tensor = get_graph_input_tensor(graph, node_idx, tensor_idx);

    if (!check_tensor_valid(input_tensor)) {
        printf("Get input node failed : node_idx: %d, tensor_idx: %d\n", node_idx, tensor_idx);
        throw "invalid input error";
    }

    int dims[] = {1, 1, IMAGE_HEIGHT, IMAGE_WIDTH};
    set_tensor_shape(input_tensor, dims, 4);
    error = prerun_graph(graph);
    if (error) {
        printf("error during prerun: %d\n", error);
        throw "prerun error";
    }
    get_input_data(environment.input_data.get(), IMAGE_HEIGHT, IMAGE_WIDTH);

    error = set_tensor_buffer(input_tensor, environment.input_data.get(), IMAGE_SIZE * sizeof(float));
    if (error) {
        printf("error during set_tensor_buffer: %d\n", error);
        throw "set_tensor_buffer";
    }

    return environment;
}

static inline int get_env_variable_int(const char *variable_name, const int default_value) {
    const char *value_str = std::getenv(variable_name);
    return (value_str) ? std::strtoul(value_str, nullptr, 0) : default_value;
}

int main(int argc, char *argv[]) {

    Config config(argc, argv);
    if (config.stop) {
        return 0;
    }

    Environment environment = init(config);

    int repeat_count(get_env_variable_int("REPEAT_COUNT", 1));
    int threads_number(get_env_variable_int("OPENBLAS_NUM_THREADS", -1));
    int sleep_seconds(get_env_variable_int("SLEEP_TIME", 0));

    for (int i = 0; i < repeat_count;) {
        auto t0 = std::chrono::system_clock::now();
        run_graph(environment.graph, 1);
        auto t1 = std::chrono::system_clock::now();

        double mytime = std::chrono::duration<double>(t1 - t0).count() * 1000;
        if (config.cpus.empty()) {
            printf("%f\t%d\n", mytime, threads_number);
        } else {
            printf("%f\t%d\t%s\n", mytime, threads_number, config.cpus.c_str());
        }
        fflush(stdout);
        if (++i < repeat_count) {
            std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
        }
    }

    return 0;
}
