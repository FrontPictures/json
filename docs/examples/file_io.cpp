#include <nlohmann/json.hpp>
#include <fstream>
#include <iterator>
#include <string>

int main()
{
    const auto original = nlohmann::json::object({{"name", "wall"}, {"port", 2}});
    {
        std::ofstream output("document.json", std::ios::binary);
        output << original.dump(2) << '\n';
        if (!output) {
            return 1;
        }
    }
    std::ifstream input("document.json", std::ios::binary);
    if (!input) {
        return 1;
    }
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    const auto decoded = nlohmann::json::parse(contents);
    return decoded == original ? 0 : 1;
}
