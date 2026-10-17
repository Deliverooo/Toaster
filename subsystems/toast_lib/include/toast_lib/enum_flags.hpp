#pragma once

#include <type_traits>

// Use once per namespace to declare flags
#define TST_DECLARE_FLAGS_FOR_NAMESPACE()\
	\
	template<typename TBitType> requires std::is_integral_v<std::underlying_type_t<TBitType>>\
	class Flags\
	{\
	public:\
		using MaskType = std::underlying_type_t<TBitType>;\
\
		constexpr Flags() noexcept : m_bitmask(0) {}\
		constexpr Flags(TBitType p_bit) noexcept : m_bitmask(static_cast<MaskType>(p_bit)) {}\
		constexpr Flags(const Flags &p_other) noexcept = default;\
		constexpr explicit Flags(MaskType p_mask) noexcept : m_bitmask(p_mask) {}\
\
		constexpr auto operator<=>(const Flags &) const = default;\
\
		constexpr auto operator!() const noexcept -> bool { return !m_bitmask; }\
		constexpr auto operator|(const Flags &p_other) const noexcept -> Flags { return Flags(m_bitmask | p_other.m_bitmask); }\
		constexpr auto operator&(const Flags &p_other) const noexcept -> Flags { return Flags(m_bitmask & p_other.m_bitmask); }\
		constexpr auto operator^(const Flags &p_other) const noexcept -> Flags { return Flags(m_bitmask ^ p_other.m_bitmask); }\
\
		constexpr auto operator~() const noexcept -> Flags { return Flags(~m_bitmask); }\
\
		constexpr Flags &operator=(const Flags &p_other)  noexcept = default;\
		constexpr Flags &operator|=(const Flags &p_other) noexcept { m_bitmask |= p_other.m_bitmask; return *this; };\
		constexpr Flags &operator&=(const Flags &p_other) noexcept { m_bitmask &= p_other.m_bitmask; return *this; };\
		constexpr Flags &operator^=(const Flags &p_other) noexcept { m_bitmask ^= p_other.m_bitmask; return *this; };\
\
		explicit constexpr operator bool() const noexcept { return !!m_bitmask; }\
		explicit constexpr operator MaskType() const noexcept { return m_bitmask; }\
\
	private:\
		MaskType m_bitmask;\
	};\
\
	template<typename TBitType>\
	struct FlagOperations {	static constexpr bool defined{false}; };\
\
	template<typename TBitType>\
	constexpr Flags<TBitType> operator|(TBitType p_bit, const Flags<TBitType> &p_flags) noexcept { return p_flags.operator|(p_bit); }\
	template<typename TBitType>\
	constexpr Flags<TBitType> operator&(TBitType p_bit, const Flags<TBitType> &p_flags) noexcept { return p_flags.operator&(p_bit); }\
	template<typename TBitType>\
	constexpr Flags<TBitType> operator^(TBitType p_bit, const Flags<TBitType> &p_flags) noexcept { return p_flags.operator^(p_bit); }\
	template<typename TBitType> requires FlagOperations<TBitType>::defined\
	constexpr Flags<TBitType> operator|(TBitType p_lhs, TBitType p_rhs) noexcept { return Flags<TBitType>{p_lhs} | p_rhs; }\
	template<typename TBitType> requires FlagOperations<TBitType>::defined\
	constexpr Flags<TBitType> operator&(TBitType p_lhs, TBitType p_rhs) noexcept { return Flags<TBitType>{p_lhs} & p_rhs; }\
	template<typename TBitType> requires FlagOperations<TBitType>::defined\
	constexpr Flags<TBitType> operator^(TBitType p_lhs, TBitType p_rhs) noexcept { return Flags<TBitType>{p_lhs} ^ p_rhs; }\
	template<typename TBitType> requires FlagOperations<TBitType>::defined\
	constexpr Flags<TBitType> operator~(TBitType p_bit) noexcept { return ~Flags<TBitType>{p_bit}; }

#define TST_SPECIALISE_FLAGS(__bit_type, __name) template<> struct FlagOperations<__bit_type> { static constexpr bool defined{true}; };\
	using __name = Flags<__bit_type>
