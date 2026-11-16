#include <nlohmann/json.hpp>
#include <string>

int main()
{
    using nlohmann::json;
    json document = json::object();
    document.reserve_object_items(8);
    document["name"] = "wall";
    document["ports"] = json::array({1, 2});
    document["enabled"] = true;

    const auto decoded = json::parse(document.dump(2));
    if (decoded != document || decoded.at("name").get<std::string>() != "wall") {
        return 1;
    }
    return decoded.at(json::json_pointer("/ports/1")).get<int>() == 2 ? 0 : 1;
}
