#ifndef ADAPTERS_INBOUND_TCP_PROTOCOL_COMMON_BYTECODEC_HPP
#define ADAPTERS_INBOUND_TCP_PROTOCOL_COMMON_BYTECODEC_HPP

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Protocol/WireTypes.hpp"

// LenPrefixed: Data with its length (ex. "name" => | 5 | "name" |)

namespace ByteCodec
{
namespace Detail
{
template <typename T>
using RemoveCvRef = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
concept EnumLike = std::is_enum_v<RemoveCvRef<T>>;

template <typename T>
concept StringLike = std::is_convertible_v<T, std::string_view>;

template <typename T>
concept ByteSpanLike = std::same_as<RemoveCvRef<T>, std::span<const std::byte>> ||
                       std::same_as<RemoveCvRef<T>, std::span<std::byte>>;

template <typename T>
concept IntegerLike = std::integral<RemoveCvRef<T>> && !std::same_as<RemoveCvRef<T>, bool>;

template <std::size_t Size>
struct UnsignedBySize;

template <>
struct UnsignedBySize<1>
{
    using type = std::uint8_t;
};

template <>
struct UnsignedBySize<2>
{
    using type = std::uint16_t;
};

template <>
struct UnsignedBySize<4>
{
    using type = std::uint32_t;
};

template <>
struct UnsignedBySize<8>
{
    using type = std::uint64_t;
};

template <std::size_t Size>
using UnsignedBySizeT = typename UnsignedBySize<Size>::type;

template <typename T>
concept FloatingLike = std::floating_point<RemoveCvRef<T>> &&
                       requires {
                           typename UnsignedBySizeT<sizeof(RemoveCvRef<T>)>;
                       };

template <typename Int>
using UnsignedOf = std::make_unsigned_t<RemoveCvRef<Int>>;

template <IntegerLike Int>
constexpr UnsignedOf<Int> ToUnsignedBits(Int value)
{
    using Value = RemoveCvRef<Int>;
    using Unsigned = UnsignedOf<Int>;

    if constexpr (std::is_unsigned_v<Value>)
        return static_cast<Unsigned>(value);
    else
        return std::bit_cast<Unsigned>(static_cast<Value>(value));
}

template <IntegerLike Int>
constexpr RemoveCvRef<Int> FromUnsignedBits(UnsignedOf<Int> raw)
{
    using Value = RemoveCvRef<Int>;

    if constexpr (std::is_unsigned_v<Value>)
        return static_cast<Value>(raw);
    else
        return std::bit_cast<Value>(raw);
}

template <FloatingLike Float>
constexpr UnsignedBySizeT<sizeof(RemoveCvRef<Float>)> ToUnsignedBits(Float value)
{
    using Value = RemoveCvRef<Float>;
    using Unsigned = UnsignedBySizeT<sizeof(Value)>;
    return std::bit_cast<Unsigned>(static_cast<Value>(value));
}

template <FloatingLike Float>
constexpr RemoveCvRef<Float> FromUnsignedBits(UnsignedBySizeT<sizeof(RemoveCvRef<Float>)> raw)
{
    using Value = RemoveCvRef<Float>;
    return std::bit_cast<Value>(raw);
}
} // namespace Detail

class Reader
{
  public:
    explicit Reader(std::span<const std::byte> payload)
        : m_payload(payload)
    {
    }

    explicit Reader(const std::vector<std::byte> &payload)
        : m_payload(payload.data(), payload.size())
    {
    }

    template <std::unsigned_integral LengthType = TCPWireTypes::DataLengthType, typename T>
    bool ReadLenPrefixed(T &out)
    {
        LengthType length = 0;
        if (!ReadUnsigned(length))
            return false;

        if constexpr (sizeof(LengthType) > sizeof(std::size_t))
        {
            if (length > static_cast<LengthType>(std::numeric_limits<std::size_t>::max()))
                return false;
        }

        return ReadLenPrefixedPayload(static_cast<std::size_t>(length), out);
    }

    template <typename T>
        requires(Detail::IntegerLike<T> || Detail::FloatingLike<T> || Detail::EnumLike<T>)
    bool Read(T &out)
    {
        return ReadPayload(out);
    }

    [[nodiscard]] bool Empty() const
    {
        return m_offset == m_payload.size();
    }

    [[nodiscard]] std::size_t Offset() const
    {
        return m_offset;
    }

    [[nodiscard]] std::size_t Remaining() const
    {
        return m_payload.size() - m_offset;
    }

  private:
    // Read network byte order (Big Endian) unsigned integer.
    template <std::unsigned_integral T>
    bool ReadUnsigned(T &out)
    {
        if (!CanRead(sizeof(T)))
            return false;

        T value = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            value = static_cast<T>((value << 8U) |
                                   static_cast<T>(std::to_integer<unsigned char>(m_payload[m_offset + i])));
        }

        m_offset += sizeof(T);
        out = value;
        return true;
    }

    bool ReadLenPrefixedPayload(std::size_t length, std::span<const std::byte> &out)
    {
        if (!CanRead(length))
            return false;

        out = m_payload.subspan(m_offset, length);
        m_offset += length;
        return true;
    }

    bool ReadLenPrefixedPayload(std::size_t length, std::vector<std::byte> &out)
    {
        if (!CanRead(length))
            return false;

        out.assign(m_payload.begin() + static_cast<std::ptrdiff_t>(m_offset),
                   m_payload.begin() + static_cast<std::ptrdiff_t>(m_offset + length));
        m_offset += length;
        return true;
    }

    bool ReadLenPrefixedPayload(std::size_t length, std::string_view &out)
    {
        if (!CanRead(length))
            return false;

        auto *ptr = reinterpret_cast<const char *>(m_payload.data() + m_offset);
        out = std::string_view(ptr, length);
        m_offset += length;
        return true;
    }

    bool ReadLenPrefixedPayload(std::size_t length, std::string &out)
    {
        std::string_view tmp;

        if (!CanRead(length))
            return false;

        auto *ptr = reinterpret_cast<const char *>(m_payload.data() + m_offset);
        tmp = std::string_view(ptr, length);
        m_offset += length;

        out.assign(tmp.data(), tmp.size());
        return true;
    }

    template <Detail::IntegerLike Int>
    bool ReadPayload(Int &out)
    {
        using Unsigned = Detail::UnsignedOf<Int>;

        Unsigned raw = 0;
        if (!ReadUnsigned(raw))
            return false;

        out = Detail::FromUnsignedBits<Int>(raw);
        return true;
    }

    template <Detail::FloatingLike Float>
    bool ReadPayload(Float &out)
    {
        using Unsigned = Detail::UnsignedBySizeT<sizeof(Detail::RemoveCvRef<Float>)>;

        Unsigned raw = 0;
        if (!ReadUnsigned(raw))
            return false;

        out = Detail::FromUnsignedBits<Float>(raw);
        return true;
    }

    template <Detail::EnumLike Enum>
    bool ReadPayload(Enum &out)
    {
        using Value = Detail::RemoveCvRef<Enum>;
        using Underlying = std::underlying_type_t<Value>;
        using Unsigned = Detail::UnsignedOf<Underlying>;

        Unsigned raw = 0;
        if (!ReadUnsigned(raw))
            return false;

        out = static_cast<Value>(Detail::FromUnsignedBits<Underlying>(raw));
        return true;
    }

    bool CanRead(std::size_t length) const
    {
        return m_offset <= m_payload.size() && m_payload.size() - m_offset >= length;
    }

    std::span<const std::byte> m_payload;
    std::size_t m_offset = 0;
};

class Writer
{
  public:
    explicit Writer(std::vector<std::byte> &payload, std::size_t reserve_additional = 0)
        : m_payload(payload)
    {
        if (reserve_additional)
            m_payload.reserve(m_payload.size() + reserve_additional);
    }

    template <std::unsigned_integral LengthType = TCPWireTypes::DataLengthType, typename T>
    bool WriteLenPrefixed(const T &value)
    {
        return WriteLenPrefixedPayload<LengthType>(value);
    }

    template <typename T>
        requires(Detail::IntegerLike<T> || Detail::FloatingLike<T> || Detail::EnumLike<T>)
    bool Write(const T &value)
    {
        return WritePayload(value);
    }

  private:
    template <std::unsigned_integral T>
    void WriteUnsigned(T value)
    {
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            auto shift = static_cast<unsigned int>((sizeof(T) - 1 - i) * 8U);
            auto byte = static_cast<unsigned char>((value >> shift) & static_cast<T>(0xFF));
            m_payload.push_back(static_cast<std::byte>(byte));
        }
    }

    template <std::unsigned_integral LengthType, Detail::StringLike T>
    bool WriteLenPrefixedPayload(const T &value)
    {
        std::string_view view = value;
        if (view.size() > static_cast<std::size_t>(std::numeric_limits<LengthType>::max()))
            return false;

        WriteUnsigned(static_cast<LengthType>(view.size()));
        auto *ptr = reinterpret_cast<const std::byte *>(view.data());
        auto bytes = std::span<const std::byte>(ptr, view.size());
        m_payload.insert(m_payload.end(), bytes.begin(), bytes.end());
        return true;
    }

    template <std::unsigned_integral LengthType, Detail::ByteSpanLike T>
    bool WriteLenPrefixedPayload(const T &value)
    {
        std::span<const std::byte> bytes(value.data(), value.size());
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<LengthType>::max()))
            return false;

        WriteUnsigned(static_cast<LengthType>(bytes.size()));
        m_payload.insert(m_payload.end(), bytes.begin(), bytes.end());
        return true;
    }

    template <std::unsigned_integral LengthType>
    bool WriteLenPrefixedPayload(const std::vector<std::byte> &value)
    {
        return WriteLenPrefixedPayload<LengthType>(std::span<const std::byte>(value.data(), value.size()));
    }

    template <Detail::IntegerLike Int>
    bool WritePayload(const Int &value)
    {
        WriteUnsigned(Detail::ToUnsignedBits(value));
        return true;
    }

    template <Detail::FloatingLike Float>
    bool WritePayload(const Float &value)
    {
        WriteUnsigned(Detail::ToUnsignedBits(value));
        return true;
    }

    template <Detail::EnumLike Enum>
    bool WritePayload(const Enum &value)
    {
        using Value = Detail::RemoveCvRef<Enum>;
        using Underlying = std::underlying_type_t<Value>;
        WriteUnsigned(Detail::ToUnsignedBits(static_cast<Underlying>(value)));
        return true;
    }

    std::vector<std::byte> &m_payload;
};
} // namespace ByteCodec

#endif /* ADAPTERS_INBOUND_TCP_PROTOCOL_COMMON_BYTECODEC_HPP */
