#ifndef __AWS__
#define __AWS__

#include "string.h"

extern const char root_cert_auth_pem_start[]        asm("_binary_root_cert_auth_pem_start");
extern const char root_cert_auth_pem_end[]          asm("_binary_root_cert_auth_pem_end");
    
extern const char client_stag_cert_pem_start[]           asm("_binary_client_stag_crt_start");
extern const char client_stag_cert_pem_end[]             asm("_binary_client_stag_crt_end");
    
extern const char client_stag_key_pem_start[]            asm("_binary_client_stag_key_start");
extern const char client_stag_key_pem_end[]              asm("_binary_client_stag_key_end");

extern const char client_prod_cert_pem_start[]           asm("_binary_client_prod_crt_start");
extern const char client_prod_cert_pem_end[]             asm("_binary_client_prod_crt_end");
    
extern const char client_prod_key_pem_start[]            asm("_binary_client_prod_key_start");
extern const char client_prod_key_pem_end[]              asm("_binary_client_prod_key_end");

/*-----------------------------------------------------------*/
#define PROVISIONING_TEMPLATE_NAME                  "Mini2_FP"
#define PROVISIONING_TEMPLATE_NAME_LENGTH           ( ( uint16_t ) ( sizeof( PROVISIONING_TEMPLATE_NAME ) - 1 ) )
/*-----------------------------------------------------------*/                                
#define FP_CREATE_CERT_API_PREFIX                   "$aws/certificates/create/"
#define FP_CREATE_CERT_API_LENGTH_PREFIX            ( ( uint16_t ) ( sizeof( FP_CREATE_CERT_API_PREFIX ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_CREATE_KEYS_API_PREFIX                   "$aws/certificates/create/"
#define FP_CREATE_KEYS_API_LENGTH_PREFIX            ( ( uint16_t ) ( sizeof( FP_CREATE_KEYS_API_PREFIX ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_REGISTER_API_PREFIX                      "$aws/provisioning-templates/"
#define FP_REGISTER_API_LENGTH_PREFIX               ( ( uint16_t ) ( sizeof( FP_REGISTER_API_PREFIX ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_REGISTER_API_BRIDGE                      "/provision/"
#define FP_REGISTER_API_LENGTH_BRIDGE               ( ( uint16_t ) ( sizeof( FP_REGISTER_API_BRIDGE ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_API_JSON_FORMAT                          "json"
#define FP_API_LENGTH_JSON_FORMAT                   ( ( uint16_t ) ( sizeof( FP_API_JSON_FORMAT ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_API_CBOR_FORMAT                          "cbor"
#define FP_API_LENGTH_CBOR_FORMAT                   ( ( uint16_t ) ( sizeof( FP_API_CBOR_FORMAT ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_API_ACCEPTED_SUFFIX                      "/accepted"
#define FP_API_LENGTH_ACCEPTED_SUFFIX               ( ( uint16_t ) ( sizeof( FP_API_ACCEPTED_SUFFIX ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_API_REJECTED_SUFFIX                      "/rejected"
#define FP_API_LENGTH_REJECTED_SUFFIX               ( ( uint16_t ) ( sizeof( FP_API_REJECTED_SUFFIX ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_PUBLISH_TOPIC \
    ( FP_CREATE_CERT_API_PREFIX           \
      FP_API_JSON_FORMAT )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_ACCEPTED_TOPIC \
    ( FP_CREATE_CERT_API_PREFIX            \
      FP_API_JSON_FORMAT                   \
      FP_API_ACCEPTED_SUFFIX )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_REJECTED_TOPIC \
    ( FP_CREATE_CERT_API_PREFIX            \
      FP_API_JSON_FORMAT                   \
      FP_API_REJECTED_SUFFIX )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_PUBLISH_LENGTH \
    ( ( uint16_t ) ( sizeof( FP_JSON_CREATE_CERT_PUBLISH_TOPIC ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_ACCEPTED_LENGTH \
    ( ( uint16_t ) ( sizeof( FP_JSON_CREATE_CERT_ACCEPTED_TOPIC ) - 1U ) )
/*-----------------------------------------------------------*/
#define FP_JSON_CREATE_CERT_REJECTED_LENGTH \
    ( ( uint16_t ) ( sizeof( FP_JSON_CREATE_CERT_REJECTED_TOPIC ) - 1U ) )     
/*-----------------------------------------------------------*/
#define FP_JSON_REGISTER_PUBLISH_TOPIC( templateName ) \
    ( FP_REGISTER_API_PREFIX                           \
      templateName                                     \
      FP_REGISTER_API_BRIDGE                           \
      FP_API_JSON_FORMAT )
/*-----------------------------------------------------------*/     
#define FP_JSON_REGISTER_ACCEPTED_TOPIC( templateName ) \
    ( FP_REGISTER_API_PREFIX                            \
      templateName                                      \
      FP_REGISTER_API_BRIDGE                            \
      FP_API_JSON_FORMAT                                \
      FP_API_ACCEPTED_SUFFIX )
/*-----------------------------------------------------------*/
#define FP_JSON_REGISTER_REJECTED_TOPIC( templateName ) \
    ( FP_REGISTER_API_PREFIX                            \
      templateName                                      \
      FP_REGISTER_API_BRIDGE                            \
      FP_API_JSON_FORMAT                                \
      FP_API_REJECTED_SUFFIX )
/*-----------------------------------------------------------*/
#define FP_JSON_REGISTER_PUBLISH_LENGTH( templateNameLength ) \
    ( FP_REGISTER_API_LENGTH_PREFIX +                         \
      ( templateNameLength ) +                                \
      FP_REGISTER_API_LENGTH_BRIDGE +                         \
      FP_API_LENGTH_JSON_FORMAT )
/*-----------------------------------------------------------*/
#define FP_JSON_REGISTER_ACCEPTED_LENGTH( templateNameLength ) \
    ( FP_REGISTER_API_LENGTH_PREFIX +                          \
      ( templateNameLength ) +                                 \
      FP_REGISTER_API_LENGTH_BRIDGE +                          \
      FP_API_LENGTH_JSON_FORMAT +                              \
      FP_API_LENGTH_ACCEPTED_SUFFIX )
/*-----------------------------------------------------------*/
#define FP_JSON_REGISTER_REJECTED_LENGTH( templateNameLength ) \
    ( FP_REGISTER_API_LENGTH_PREFIX +                          \
      ( templateNameLength ) +                                 \
      FP_REGISTER_API_LENGTH_BRIDGE +                          \
      FP_API_LENGTH_JSON_FORMAT +                              \
      FP_API_LENGTH_REJECTED_SUFFIX )
/*-----------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void AWSTask(void *arg);
void AWSRun(void);
void AWSStop(void);

#ifdef __cplusplus
}
#endif

#endif //__AWS__
