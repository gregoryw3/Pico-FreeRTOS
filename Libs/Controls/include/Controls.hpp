#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <libfixmath/libfixmath/fix16.hpp>
#include <libfixmath/libfixmath/fix16.h>

#include "PID.hpp"

namespace Controls {
    /**
     * @brief Controls Library - Collection of control system components
     * 
     * This library provides various control system implementations:
     * - PID Controllers (see PID namespace)
     * - State Observers (Luenberger namespace)
     * - Digital Filters (Filters namespace)
     */

    namespace Luenberger {
        // Forward declarations for state observers
        template<typename T, size_t StateSize, size_t InputSize, size_t OutputSize>
        class Observer;
    }

    namespace Filters {
        namespace LowPass {
            // Low-pass filter implementation
        }
        namespace HighPass {
            // High-pass filter implementation
        }
        namespace BandPass {
            // Band-pass filter implementation
        }
        namespace Notch {
            // Notch filter implementation
        }
        namespace Complementary {
            // Complementary filter implementation
        }

    }
}