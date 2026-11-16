#include <nlohmann/json.hpp>
#include <nlohmann/detail/output/output_adapters.hpp>
#include <nlohmann/detail/output/serializer.hpp>
#include <string>

int main()
{
    std::string output;
    nlohmann::detail::serializer writer(nlohmann::detail::output_adapter(output), ' ');
    const auto indent = writer.dumpObjectBegin(true, 2, 0, true);
    writer.dumpKey("name", true, indent, false, true);
    writer.dumpString("wall");
    writer.dumpObjectComa(true, true);
    writer.dumpKey("ports", true, indent, false, true);
    writer.dump(nlohmann::json::array({1, 2}), true, false, 2);
    writer.dumpObjectEnd(true, 0, true);

    const auto expected = nlohmann::json::object({{"name", "wall"},
                                                {"ports", nlohmann::json::array({1, 2})}});
    return output == "{\"name\": \"wall\", \"ports\": [1, 2]}"
                   && nlohmann::json::parse(output) == expected ? 0 : 1;
}
