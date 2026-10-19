#pragma once

struct StringHash {
	using is_transparent = void;

	static constexpr size_t operator()(const std::string_view value) noexcept {
		return std::hash<std::string_view>{}(value);
	}

};

struct StringEqual {
	using is_transparent = void;

	static constexpr bool operator()(const std::string_view lhs, const std::string_view rhs) noexcept {
		return lhs == rhs;
	}
};
