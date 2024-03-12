#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctime>
#include <random>
#include <string>
#include <thread> // NOLINT
#include <vector>


// #include "gemma.h" // Adjust include path as necessary

#include "compression/compress.h"
#include "gemma.h" // Gemma
#include "util/app.h"
#include "util/args.h" // HasHelp
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 4096;

namespace gcpp
{

    void ShowHelp(gcpp::LoaderArgs &loader, gcpp::InferenceArgs &inference,
                  gcpp::AppArgs &app)
    {
        fprintf(stderr,
                "\ngemma.cpp\n---------\n\nTo run gemma.cpp, you need to "
                "specify 3 required model loading arguments: --tokenizer, "
                "--compressed_weights, "
                "and --model.\n\nModel Loading Arguments\n\n");
        loader.Help();
        fprintf(stderr, "\nInference Arguments\n\n");
        inference.Help();
        fprintf(stderr, "\nApplication Arguments\n\n");
        app.Help();
        fprintf(stderr, "\n\n");
    }

    void ShowConfig(LoaderArgs &loader, InferenceArgs &inference, AppArgs &app)
    {
        loader.Print(app.verbosity);
        inference.Print(app.verbosity);
        app.Print(app.verbosity);

        if (app.verbosity >= 2)
        {
            time_t now = time(nullptr);
            char *dt = ctime(&now); // NOLINT
            std::cout << "Date & Time                   : " << dt
                      << "Prefill Token Batch Size      : " << gcpp::kPrefillBatchSize
                      << "\n"
                      << "Hardware concurrency          : "
                      << std::thread::hardware_concurrency() << std::endl
                      << "Instruction set               : "
                      << hwy::TargetName(hwy::DispatchedTarget()) << " ("
                      << hwy::VectorBytes() * 8 << " bits)"
                      << "\n"
                      << "Weight Type                   : "
                      << gcpp::TypeName(gcpp::WeightT()) << "\n"
                      << "EmbedderInput Type            : "
                      << gcpp::TypeName(gcpp::EmbedderInputT()) << "\n";
        }
    }

    std::string decode(gcpp::Gemma &model, hwy::ThreadPool &pool,
                   hwy::ThreadPool &inner_pool, const InferenceArgs &args,
                   int verbosity, const gcpp::AcceptFunc &accept_token, std::string &prompt_string) 
    {
        std::string generated_text;
        // Seed the random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        if (model.model_training == ModelTraining::GEMMA_IT)
            {
                // For instruction-tuned models: add control tokens.
                prompt_string = "<start_of_turn>user\n" + prompt_string +
                                "<end_of_turn>\n<start_of_turn>model\n";
            }
        // Encode the prompt string into tokens
        std::vector<int> prompt;
        HWY_ASSERT(model.Tokenizer().Encode(prompt_string, &prompt).ok());
        std::cout << "input prompt : \n" << prompt_string << std::endl;
        std::cout << "numbers of input prompt tokens : " << prompt.size() << std::endl;
        // Placeholder for generated token IDs
        std::vector<int> generated_tokens;
        // Define lambda for token decoding
        StreamFunc stream_token = [&generated_tokens](int token, float /* probability */) -> bool {
            generated_tokens.push_back(token);
            return true; // Continue generating
        };
        // Decode tokens
        GenerateGemma(model, args, prompt, /*start_pos=*/0, pool, inner_pool, stream_token, accept_token, gen, verbosity);
        HWY_ASSERT(model.Tokenizer().Decode(generated_tokens, &generated_text).ok());
        generated_text = generated_text.substr(prompt_string.size());
        std::cout << "numbers of generated text tokens : " << generated_tokens.size() << std::endl;
    return generated_text;
    }

} // namespace gcpp


int server(gcpp::LoaderArgs &loader, gcpp::InferenceArgs &inference, gcpp::AppArgs &app) {
    // 创建套接字
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error: Could not create socket.\n";
        return 1;
    }

    // 准备地址结构体
    struct sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "Error: Could not bind socket.\n";
        close(server_socket);
        return 1;
    }

    // 监听连接
    if (listen(server_socket, 5) == -1) {
        std::cerr << "Error: Could not listen on socket.\n";
        close(server_socket);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // 接受连接
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

   
    // 接收消息
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    ssize_t n;
    
    hwy::ThreadPool inner_pool(0);
    hwy::ThreadPool pool(app.num_threads);
    if (app.num_threads > 10)
    {
        gcpp::PinThreadToCore(app.num_threads - 1); // Main thread

        pool.Run(0, pool.NumThreads(),
                    [](uint64_t /*task*/, size_t thread)
                    { gcpp::PinThreadToCore(thread); });
    }
    gcpp::Gemma model(loader, pool);

    std::string generated_text;
    std::string prompt;

    while(true){
        client_socket = accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_address), &client_address_len);
        if (client_socket == -1) {
            std::cerr << "Error: Could not accept incoming connection.\n";
            close(server_socket);
            return 1;
        }

        std::cout << "Client connected." << std::endl;

        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        buffer[bytes_received] = '\0';
        // std::cout << "Received: " << buffer << std::endl;
        prompt.assign(buffer);
        generated_text = gcpp::decode(model, pool, inner_pool, inference, app.verbosity, /*accept_token=*/[](int)
                { return true; }, prompt);
        strcpy(buffer, generated_text.c_str());
        n = send(client_socket, buffer, strlen(buffer), 0);
        // std::cout << "Confirmation code:  " << n << std::endl;
    }
    
    // 关闭套接字
    close(client_socket);
    close(server_socket);

    return 0;

}

int completion_base(int argc, char **argv)
{   
    gcpp::LoaderArgs loader(argc, argv);
    gcpp::InferenceArgs inference(argc, argv);
    gcpp::AppArgs app(argc, argv);

    if (gcpp::HasHelp(argc, argv)) {
      ShowHelp(loader, inference, app);
      return 0;
    }

    if (const char* error = loader.Validate()) {
      ShowHelp(loader, inference, app);
      HWY_ABORT("\nInvalid args: %s", error);
    }

    return server(loader, inference, app);
}

int main(int argc, char** argv) {
    completion_base(argc, argv);
    return 0;
}
