msvc:DEFINES *= _SCL_SECURE_NO_WARNINGS

# swift standard warnings
msvc:QMAKE_CXXFLAGS_WARN_ON *= /wd4351 /wd4661
clang_cl:QMAKE_CXXFLAGS_WARN_ON *= /W4 -Wno-unknown-pragmas -Wno-undefined-inline -Wno-self-assign-overloaded
gcc:QMAKE_CXXFLAGS_WARN_ON *= -Woverloaded-virtual
gcc:QMAKE_CXXFLAGS_USE_PRECOMPILE = -Winvalid-pch $$QMAKE_CXXFLAGS_USE_PRECOMPILE

# elevated warnings
swiftConfig(allowNoisyWarnings) {
    clang {
        QMAKE_CXXFLAGS_WARN_ON -= -Wno-self-assign-overloaded
        QMAKE_CXXFLAGS_WARN_ON *= -Weverything --system-header-prefix=$$[QT_INSTALL_HEADERS]
        QMAKE_CXXFLAGS_WARN_ON += -Wno-system-headers -Wno-c++98-compat-pedantic -Wno-class-varargs -Wno-covered-switch-default
        QMAKE_CXXFLAGS_WARN_ON += -Wno-documentation -Wno-documentation-unknown-command -Wno-double-promotion -Wno-exit-time-destructors
        QMAKE_CXXFLAGS_WARN_ON += -Wno-gnu -Wno-missing-prototypes -Wno-newline-eof -Wno-padded -Wno-undefined-reinterpret-cast
        QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-macros -Wno-used-but-marked-unused -Wno-weak-template-vtables
    }
    else:msvc {
        QMAKE_CXXFLAGS_WARN_ON *= /Wall
        QMAKE_CXXFLAGS_WARN_ON += /wd4640 /wd4619 /wd4350 /wd4351 /wd4946 /wd4510 /wd4820 /wd4571 /wd4625 /wd4626 /wd4127

        DEFINES *= QT_CC_WARNINGS
        QMAKE_CXXFLAGS_WARN_ON *= /wd4251 /wd4275 /wd4514 /wd4800 /wd4097 /wd4706 /wd4710 /wd4530
    }
}

# gcc 5 can warn about missing override keyword,
# gcc 6 can do it without thousands of warnings in qt headers
gcc {
    GCC_VERSION = $$system($$QMAKE_CXX -dumpversion)
    GCC_MAJOR = $$section(GCC_VERSION, ., 0, 0)
    greaterThan(GCC_MAJOR, 5) {
        QMAKE_CXXFLAGS_WARN_ON *= -Wsuggest-override
    }
    greaterThan(GCC_MAJOR, 4):swiftConfig(allowNoisyWarnings) {
        QMAKE_CXXFLAGS_WARN_ON *= -Wsuggest-override
    }
}

# clazy - Qt-aware linter
equals(QMAKE_CXX, clazy)|equals(QMAKE_CXX, clazy-cl) {
    CLAZY_WARNINGS *= level3 no-reserve-candidates

    # TODO: gradually fix issues so we can re-enable some of these warnings
    CLAZY_WARNINGS *= no-inefficient-qlist-soft no-qstring-allocations
    CLAZY_WARNINGS *= no-missing-qobject-macro no-ctor-missing-parent-argument
    CLAZY_WARNINGS *= no-copyable-polymorphic no-function-args-by-value

    QMAKE_CXXFLAGS_WARN_ON += -Xclang -plugin-arg-clang-lazy -Xclang $$join(CLAZY_WARNINGS, ",")
}
