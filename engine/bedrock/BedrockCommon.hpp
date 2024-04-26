#pragma once

#define MFA_VARIABLE1(variable, type, default, pre)             \
protected:                                                      \
type pre##variable = default;                                   \
public:                                                         \
void Set##variable(type value)                                  \
{                                                               \
    if (pre##variable == value)                                 \
    {                                                           \
        return;                                                 \
    }                                                           \
    pre##variable = value;                                      \
}                                                               \
[[nodiscard]]                                                   \
type Get##variable() const                                      \
{                                                               \
    return pre##variable;                                       \
}                                                               \
protected:                                                      \

#define MFA_VARIABLE2(variable, type, default, onChange, pre)   \
protected:                                                      \
type pre##variable = default;                                   \
public:                                                         \
void Set##variable(type value)                                  \
{                                                               \
    if (pre##variable == value)                                 \
    {                                                           \
        return;                                                 \
    }                                                           \
    pre##variable = value;                                      \
    onChange();                                                 \
}                                                               \
[[nodiscard]]                                                   \
type Get##variable() const                                      \
{                                                               \
    return pre##variable;                                       \
}                                                               \
protected:                                                      \


#define MFA_VARIABLE3(variable, type, default, onChange, pre)   \
protected:                                                      \
type pre##variable = default;                                   \
public:                                                         \
void Set##variable(type value)                                  \
{                                                               \
    if (pre##variable == value)                                 \
    {                                                           \
        return;                                                 \
    }                                                           \
    pre##variable = value;                                      \
    onChange();                                                 \
}                                                               \
[[nodiscard]]                                                   \
type const & Get##variable() const                              \
{                                                               \
    return pre##variable;                                       \
}                                                               \
protected:                                                      \


#define MFA_UNIQUE_NAME(base_) MFA_CONCAT(base_, __COUNTER__)

#define MFA_CONCAT__IMPL(x_, y_) x_ ## y_
#define MFA_CONCAT(x_, y_) MFA_CONCAT__IMPL(x_, y_)
