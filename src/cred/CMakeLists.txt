add_library(cred Cache.cpp Store.cpp CredentialHelper.cpp GitCredential.cpp)

target_link_libraries(cred conf git Qt6::Core)
add_library(cred Cache.cpp CredentialHelper.cpp GitCredential.cpp
                 ${CREDENTIAL_IMPL_FILE})

set_target_properties(cred PROPERTIES AUTOMOC ON)
