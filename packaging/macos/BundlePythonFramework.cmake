# Copyright (C) 2026 Paul McKinney
# BundlePythonFramework.cmake
# Copies Python.framework into the app bundle's Frameworks/ directory and fixes
# install names so the embedded interpreter is fully self-contained.
#
# Required variables (passed via -D on the cmake -P command line):
#   PYTHON_FRAMEWORK_SRC — root of the source Python.framework
#                          (e.g. /Library/Frameworks/Python.framework)
#   FRAMEWORKS           — path to Contents/Frameworks inside the built app bundle
#   EXECUTABLE           — path to the app's main executable (Contents/MacOS/ProjectNotes)
#   PYTHON_VERSION       — major.minor string, e.g. "3.14"

cmake_minimum_required(VERSION 3.16)

set(_dest_framework "${FRAMEWORKS}/Python.framework")
set(_py_dylib       "${_dest_framework}/Versions/${PYTHON_VERSION}/Python")
set(_old_id         "/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python")
set(_new_id         "@rpath/Python.framework/Versions/${PYTHON_VERSION}/Python")

# ── 1. Copy Python.framework, preserving internal symlinks ───────────────────
# rsync -a preserves symlinks. Internal framework symlinks (Versions/Current,
# top-level aliases) are valid inside a bundle — only outward-pointing symlinks
# are rejected during notarization.
message(STATUS "BundlePythonFramework: copying ${PYTHON_FRAMEWORK_SRC} → ${_dest_framework}")
execute_process(
    COMMAND rsync -a --delete
        "${PYTHON_FRAMEWORK_SRC}/"
        "${_dest_framework}/"
    RESULT_VARIABLE _rsync_result
    OUTPUT_QUIET
)
if(NOT _rsync_result EQUAL 0)
    message(FATAL_ERROR "BundlePythonFramework: rsync failed (exit ${_rsync_result}). "
        "Ensure rsync is installed and PYTHON_FRAMEWORK_SRC is correct.")
endif()

# Verify the dylib actually landed where expected.
if(NOT EXISTS "${_py_dylib}")
    message(FATAL_ERROR "BundlePythonFramework: Python dylib not found at ${_py_dylib} "
        "after copy. Check PYTHON_VERSION (got '${PYTHON_VERSION}') and the "
        "framework layout at ${PYTHON_FRAMEWORK_SRC}.")
endif()

# ── 2. Fix the dylib's own install name ──────────────────────────────────────
# The original install name is an absolute system path; change it to @rpath so
# the dynamic linker resolves it relative to the executable's rpath list.
message(STATUS "BundlePythonFramework: setting install name → ${_new_id}")
execute_process(COMMAND install_name_tool -id "${_new_id}" "${_py_dylib}"
    RESULT_VARIABLE _rc OUTPUT_QUIET ERROR_QUIET)
if(NOT _rc EQUAL 0)
    message(WARNING "BundlePythonFramework: install_name_tool -id returned ${_rc}")
endif()

# ── 3. Patch the executable's reference to Python ────────────────────────────
# Find whatever Python.framework path the linker recorded and rewrite it to the
# @rpath variant. The linker may have used the exact RPATH set in Python3_ROOT_DIR
# or a slightly different canonical path, so we scan rather than assume.
execute_process(COMMAND otool -L "${EXECUTABLE}"
    OUTPUT_VARIABLE _links OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

set(_patched FALSE)
string(REPLACE "\n" ";" _lines "${_links}")
foreach(_line ${_lines})
    if(_line MATCHES "Python\\.framework")
        string(REGEX REPLACE "^[ \t]*([^ \t(]+).*" "\\1" _old_path "${_line}")
        string(STRIP "${_old_path}" _old_path)
        if(_old_path AND NOT "${_old_path}" STREQUAL "${_new_id}")
            message(STATUS "BundlePythonFramework: patching executable: ${_old_path} → ${_new_id}")
            execute_process(COMMAND install_name_tool
                -change "${_old_path}" "${_new_id}" "${EXECUTABLE}"
                OUTPUT_QUIET ERROR_QUIET)
            set(_patched TRUE)
        endif()
    endif()
endforeach()

if(NOT _patched)
    message(STATUS "BundlePythonFramework: no Python.framework reference found in executable "
        "(may already be correct or Python linked differently).")
endif()

# ── 4. Add rpath so the executable resolves @rpath/Python.framework/… ────────
execute_process(COMMAND otool -l "${EXECUTABLE}"
    OUTPUT_VARIABLE _otool_out OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT _otool_out MATCHES "@executable_path/../Frameworks")
    message(STATUS "BundlePythonFramework: adding rpath @executable_path/../Frameworks")
    execute_process(COMMAND install_name_tool
        -add_rpath "@executable_path/../Frameworks" "${EXECUTABLE}"
        OUTPUT_QUIET ERROR_QUIET)
endif()

# ── 5. Re-sign the Python dylib ───────────────────────────────────────────────
# install_name_tool invalidates the existing code signature. On Apple Silicon
# with SIP, loading a dylib with an invalid signature causes an immediate SIGKILL.
# Ad-hoc signing ("-") is sufficient for dev/build time; the final installer
# build will replace this with a proper Developer ID signature.
message(STATUS "BundlePythonFramework: re-signing Python dylib (ad-hoc)")
execute_process(COMMAND codesign --force --sign - "${_py_dylib}"
    OUTPUT_QUIET ERROR_QUIET)

message(STATUS "BundlePythonFramework: done.")
