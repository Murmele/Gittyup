add_library(cred Cache.cpp CredentialHelper.cpp GitCredential.cpp
                 ${CREDENTIAL_IMPL_FILE})

target_link_libraries(cred conf git Qt6::Core)

set_target_properties(cred PROPERTIES AUTOMOC ON)
