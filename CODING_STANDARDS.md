function defintions should be placed on a single line unless it exceeds 130 characters
function definitions should have the opening curly brace placed at the end of the definition sperated by a single space
all variable definitions in function definitions should be prefixed with 1 underscore
all varaiable names of structs and classes should be prefixed with 2 underscores
use K&R style curly brace placing for all code blocks


static std::pair<bool, std::optional<std::string>> parse_scheme(const std::string& _s, bool _validate_only) {
    if(s.empty() || !std::isalpha((unsigned char)s[0])) {
        return {false, std::nullopt};
    }

    auto i = size_t{0};

    while (i < s.size() &&
           (std::isalnum((unsigned char)s[i]) ||
            s[i] == '+' || s[i] == '-' || s[i] == '.')) {
        i++;
    }

    if (i < s.size() && s[i] == ':') {
        if (validate_only) {
            return {true, std::nullopt};
        }

        auto scheme = s.substr(0, i);

        for (auto& c : scheme) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        return {true, std::move(scheme)};
    }

    return {false, std::nullopt};
}