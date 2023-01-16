#
# Copyright (c) 2020 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set(CHIP_APP_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})

if (NOT CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CHIP_APP_BASE_DIR}/../.. REALPATH)
endif()

include("${CHIP_ROOT}/build/chip/chip_codegen.cmake")
include("${CHIP_ROOT}/src/app/codegen-data-model-provider/model.cmake")

# Configure ${APP_TARGET} with source files associated with ${CLUSTER} cluster
#
function(chip_configure_cluster APP_TARGET CLUSTER)
    file(GLOB CLUSTER_SOURCES "${CHIP_APP_BASE_DIR}/clusters/${CLUSTER}/*.cpp")
    target_sources(${APP_TARGET} PRIVATE ${CLUSTER_SOURCES})
endfunction()

#
# Configure ${APP_TARGET} with source files associated with clusters enabled in the ${ZAP_FILE}
#
function(chip_configure_zap_file APP_TARGET ZAP_FILE EXTERNAL_CLUSTERS)
    find_package(Python3 REQUIRED)
    set(args --zap_file ${ZAP_FILE})

    if(EXTERNAL_CLUSTERS)
        list(APPEND args --external-clusters ${EXTERNAL_CLUSTERS})
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CHIP_APP_BASE_DIR}/zap_cluster_list.py ${args}
        OUTPUT_VARIABLE CLUSTER_LIST
        ERROR_VARIABLE ERROR_MESSAGE
        RESULT_VARIABLE RC
    )

    if(NOT RC EQUAL 0)
        message(FATAL_ERROR "Failed to execute zap_cluster_list.py: ${ERROR_MESSAGE}")
    endif()

    string(REPLACE "\n" ";" CLUSTER_LIST "${CLUSTER_LIST}")

    foreach(CLUSTER ${CLUSTER_LIST})
        chip_configure_cluster(${APP_TARGET} ${CLUSTER})
    endforeach()
endfunction()

#
# Configure ${APP_TARGET} based on the selected data model configuration.
# Available options are:
#   SCOPE           CMake scope keyword that defines the scope of included sources.
#                   The default is PRIVATE scope.
#   BYPASS_IDL      Bypass code generation from .matter IDL file.
#   ZAP_FILE        Path to the ZAP file, used to determine the list of clusters
#                   supported by the application.
#   IDL             .matter IDL file to use for codegen. Inferred from ZAP_FILE
#                   if not provided
#   EXTERNAL_CLUSTERS Clusters with external implementations. The default implementations
#                   will not be used nor required for these clusters.
#                   Format: MY_CUSTOM_CLUSTER'.
#
function(chip_configure_data_model APP_TARGET)
    set(SCOPE PRIVATE)
    cmake_parse_arguments(ARG "BYPASS_IDL" "SCOPE;ZAP_FILE;GEN_DIR;IDL" "EXTERNAL_CLUSTERS" ${ARGN})

    if(ARG_SCOPE)
        set(SCOPE ${ARG_SCOPE})
    endif()

    # CMAKE data model auto-includes the server side implementation
    target_sources(${APP_TARGET} ${SCOPE}
        ${CHIP_APP_BASE_DIR}/server/AclStorage.cpp
        ${CHIP_APP_BASE_DIR}/server/DefaultAclStorage.cpp
        ${CHIP_APP_BASE_DIR}/server/CommissioningWindowManager.cpp
        ${CHIP_APP_BASE_DIR}/server/Dnssd.cpp
        ${CHIP_APP_BASE_DIR}/server/EchoHandler.cpp
        ${CHIP_APP_BASE_DIR}/server/OnboardingCodesUtil.cpp
        ${CHIP_APP_BASE_DIR}/server/Server.cpp
    )

    target_compile_options(${APP_TARGET} ${SCOPE}
        "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>"
    )

    if(ARG_ZAP_FILE)
        chip_configure_zap_file(${APP_TARGET} ${ARG_ZAP_FILE} "${ARG_EXTERNAL_CLUSTERS}")

        if(NOT ARG_IDL)
            string(REPLACE ".zap" ".matter" ARG_IDL ${ARG_ZAP_FILE})
        endif()
    endif()

    if (ARG_IDL AND NOT ARG_BYPASS_IDL)
        chip_codegen(${APP_TARGET}-codegen
            INPUT "${ARG_IDL}"
            GENERATOR "cpp-app"
            OUTPUTS
            "app/PluginApplicationCallbacks.h"
            "app/callback-stub.cpp"
            "app/cluster-init-callback.cpp"
            OUTPUT_PATH APP_GEN_DIR
            OUTPUT_FILES APP_GEN_FILES
        )

        target_include_directories(${APP_TARGET} ${SCOPE} "${APP_GEN_DIR}")
        add_dependencies(${APP_TARGET} ${APP_TARGET}-codegen)

        chip_zapgen(${APP_TARGET}-zapgen
            INPUT "${ARG_ZAP_FILE}"
            GENERATOR "app-templates"
            OUTPUTS
            "zap-generated/access.h"
            "zap-generated/CHIPClientCallbacks.h"
            "zap-generated/endpoint_config.h"
            "zap-generated/gen_config.h"
            "zap-generated/IMClusterCommandHandler.cpp"
            OUTPUT_PATH APP_TEMPLATES_GEN_DIR
            OUTPUT_FILES APP_TEMPLATES_GEN_FILES
        )
        target_include_directories(${APP_TARGET} ${SCOPE} "${APP_TEMPLATES_GEN_DIR}")
        add_dependencies(${APP_TARGET} ${APP_TARGET}-zapgen)
    else()
        target_compile_definitions(${APP_TARGET} PRIVATE CHIP_BYPASS_IDL)
        target_include_directories(${APP_TARGET} ${SCOPE} ${ARG_GEN_DIR})
        set(APP_GEN_FILES
            ${ARG_GEN_DIR}/callback-stub.cpp
            ${ARG_GEN_DIR}/IMClusterCommandHandler.cpp
        )
    endif()

    # These are:
    #   //src/app/icd/server:notfier
    #   //src/app/icd/server:monitoring-table
    #   //src/app/icd/server:configuration-data
    #
    # TODO: ideally we would avoid duplication and would link gn-built items. In this case
    #       it may be slightly harder because these are source_sets rather than libraries.
    target_sources(${APP_TARGET} ${SCOPE}
        ${CHIP_APP_BASE_DIR}/icd/server/ICDMonitoringTable.cpp
        ${CHIP_APP_BASE_DIR}/icd/server/ICDNotifier.cpp
        ${CHIP_APP_BASE_DIR}/icd/server/ICDConfigurationData.cpp
    )

    # This is:
    #    //src/app/common:cluster-objects
    #
    # TODO: ideally we would avoid duplication and would link gn-built items
    target_sources(${APP_TARGET} ${SCOPE}
        ${CHIP_APP_BASE_DIR}/../../zzz_generated/app-common/app-common/zap-generated/cluster-objects.cpp
    )

    target_sources(${APP_TARGET} ${SCOPE}
        ${CHIP_APP_BASE_DIR}/../../zzz_generated/app-common/app-common/zap-generated/attributes/Accessors.cpp
        ${CHIP_APP_BASE_DIR}/reporting/reporting.cpp
        ${CHIP_APP_BASE_DIR}/util/attribute-storage.cpp
        ${CHIP_APP_BASE_DIR}/util/attribute-table.cpp
        ${CHIP_APP_BASE_DIR}/util/binding-table.cpp
        ${CHIP_APP_BASE_DIR}/util/DataModelHandler.cpp
        ${CHIP_APP_BASE_DIR}/util/ember-compatibility-functions.cpp
        ${CHIP_APP_BASE_DIR}/util/ember-global-attribute-access-interface.cpp
        ${CHIP_APP_BASE_DIR}/util/ember-io-storage.cpp
        ${CHIP_APP_BASE_DIR}/util/generic-callback-stubs.cpp
        ${CHIP_APP_BASE_DIR}/util/privilege-storage.cpp
        ${CHIP_APP_BASE_DIR}/util/util.cpp
        ${CODEGEN_DATA_MODEL_SOURCES}
        ${APP_GEN_FILES}
        ${APP_TEMPLATES_GEN_FILES}
    )
endfunction()
