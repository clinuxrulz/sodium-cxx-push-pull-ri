#ifndef _SODIUM_FINALLY_H_
#define _SODIUM_FINALLY_H_

#include <functional>

namespace sodium {
    template <typename TCode, typename TFinallyCode>
    inline typename std::result_of<TCode()>::type with_finally(const TCode &code, const TFinallyCode &finally_code)
    {
        typename std::result_of<TCode()>::type result;
        try
        {
            result = code();
        }
        catch (...)
        {
            try
            {
                finally_code();
            }
            catch (...) // Maybe stupid check that finally_code mustn't throw.
            {
                std::terminate();
            }
            throw;
        }
        finally_code();
        return result;
    }

    template <typename TCode, typename TFinallyCode>
    inline void with_finally_void(const TCode &code, const TFinallyCode &finally_code)
    {
        try
        {
            code();
        }
        catch (...)
        {
            try
            {
                finally_code();
            }
            catch (...) // Maybe stupid check that finally_code mustn't throw.
            {
                std::terminate();
            }
            throw;
        }
        finally_code();
    }
}

#endif // _SODIUM_FINALLY_H_
