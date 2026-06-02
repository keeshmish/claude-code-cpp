#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (argc < 3 || std::string(argv[1]) != "-p")
    {
        std::cerr << "Expected first argument to be '-p'" << std::endl;
        return 1;
    }

    std::string prompt = argv[2];

    if (prompt.empty())
    {
        std::cerr << "Prompt must not be empty" << std::endl;
        return 1;
    }

    const char *api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char *base_url_env = std::getenv("OPENROUTER_BASE_URL");

    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty())
    {
        std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
        return 1;
    }
    json messages = json::array({{{"role", "user"}, {"content", prompt}}});
    while (true)
    {
        json request_body = {
            {"model", "anthropic/claude-haiku-4.5"},
            {"messages", messages},
            {"tools", json::array({
                {
                {"type", "function"},
                                    {"function", {{"name", "Read"}, 
                                                    {"description", "Read and return the contents of a file"}, 
                                                            {"parameters", 
                                                                 {{"type", "object"}, {"properties", 
                                                                    {{"file_path", {{"type", "string"}, {"description", "The path to the file to read"}}}}}, 
                                                                    {"required", json::array({"file_path"})}

                                                                                                                                              }}}}},  
               {
                {"type", "function"},
                                    {"function", {{"name", "Write"}, 
                                                    {"description", "Write content to a file"}, 
                                                            {"parameters", 
                                                                 {{"type", "object"}, {"properties", 
                                                                    {{"file_path", {{"type", "string"}, {"description", "The path to the file to write to"}}},
                                                                     {"content", {{"type", "string"}, {"description", "The content to write to the file"}}}}}, 
                                                                    {"required", json::array({"file_path", "content"})}
                                                                                                                                              }}}}})}};

        cpr::Response response = cpr::Post(cpr::Url{base_url + "/chat/completions"}, cpr::Header{{"Authorization", "Bearer " + api_key}, {"Content-Type", "application/json"}}, cpr::Body{request_body.dump()});

        if (response.status_code != 200)
        {
            std::cerr << "HTTP error: " << response.status_code << std::endl;
            return 1;
        }

        json result = json::parse(response.text);

        if (!result.contains("choices") || result["choices"].empty())
        {
            std::cerr << "No choices in response" << std::endl;
            return 1;
        }
        json message = result["choices"][0]["message"];

        messages.push_back(message);

        if (message.contains("tool_calls") && !message["tool_calls"].empty())
        {
            for (const auto &tc : message["tool_calls"])
            {
                string name = tc["function"]["name"];
                string args = tc["function"]["arguments"];
                string content;
                if (name == "Read")
                {
                    string file_path = json::parse(args)["file_path"];
                    ifstream file(file_path);
                    if (!file)
                    {
                        content = "error: could not open file" + file_path;
                    }
                    else
                    {
                        content = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
                    }
                }

                else if (name == "Write")
                {
                    json parsed = json::parse(args);
                    string file_path = parsed["file_path"];
                    string file_content = parsed["content"];

                    ofstream file(file_path);
                    file << file_content;
                    file.close();
                    content = "File written successfully";
                }

                messages.push_back({
                    {"role", "tool"},
                    {"tool_call_id", tc["id"].get<string>()},
                    {"content", content},
                });
            }
        }
        else
        {
            cout << message["content"].get<string>();
            break;
        }
    }
    return 0;
}
