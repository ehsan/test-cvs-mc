/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Red Hat, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Kai Engert <kengert@redhat.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsNSSComponent.h"
#include "secerr.h"
#include "sslerr.h"

const char *
nsNSSErrors::getDefaultErrorStringName(PRInt32 err)
{
  const char *id_str = nsnull;

  switch (err)
  {
    case SSL_ERROR_EXPORT_ONLY_SERVER: id_str = "SSL_ERROR_EXPORT_ONLY_SERVER"; break;
    case SSL_ERROR_US_ONLY_SERVER: id_str = "SSL_ERROR_US_ONLY_SERVER"; break;
    case SSL_ERROR_NO_CYPHER_OVERLAP: id_str = "SSL_ERROR_NO_CYPHER_OVERLAP"; break;
    case SSL_ERROR_NO_CERTIFICATE: id_str = "SSL_ERROR_NO_CERTIFICATE"; break;
    case SSL_ERROR_BAD_CERTIFICATE: id_str = "SSL_ERROR_BAD_CERTIFICATE"; break;
    case SSL_ERROR_BAD_CLIENT: id_str = "SSL_ERROR_BAD_CLIENT"; break;
    case SSL_ERROR_BAD_SERVER: id_str = "SSL_ERROR_BAD_SERVER"; break;
    case SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE: id_str = "SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE"; break;
    case SSL_ERROR_UNSUPPORTED_VERSION: id_str = "SSL_ERROR_UNSUPPORTED_VERSION"; break;
    case SSL_ERROR_WRONG_CERTIFICATE: id_str = "SSL_ERROR_WRONG_CERTIFICATE"; break;
    case SSL_ERROR_BAD_CERT_DOMAIN: id_str = "SSL_ERROR_BAD_CERT_DOMAIN"; break;
    case SSL_ERROR_SSL2_DISABLED: id_str = "SSL_ERROR_SSL2_DISABLED"; break;
    case SSL_ERROR_BAD_MAC_READ: id_str = "SSL_ERROR_BAD_MAC_READ"; break;
    case SSL_ERROR_BAD_MAC_ALERT: id_str = "SSL_ERROR_BAD_MAC_ALERT"; break;
    case SSL_ERROR_BAD_CERT_ALERT: id_str = "SSL_ERROR_BAD_CERT_ALERT"; break;
    case SSL_ERROR_REVOKED_CERT_ALERT: id_str = "SSL_ERROR_REVOKED_CERT_ALERT"; break;
    case SSL_ERROR_EXPIRED_CERT_ALERT: id_str = "SSL_ERROR_EXPIRED_CERT_ALERT"; break;
    case SSL_ERROR_SSL_DISABLED: id_str = "SSL_ERROR_SSL_DISABLED"; break;
    case SSL_ERROR_FORTEZZA_PQG: id_str = "SSL_ERROR_FORTEZZA_PQG"; break;
    case SSL_ERROR_UNKNOWN_CIPHER_SUITE: id_str = "SSL_ERROR_UNKNOWN_CIPHER_SUITE"; break;
    case SSL_ERROR_NO_CIPHERS_SUPPORTED: id_str = "SSL_ERROR_NO_CIPHERS_SUPPORTED"; break;
    case SSL_ERROR_BAD_BLOCK_PADDING: id_str = "SSL_ERROR_BAD_BLOCK_PADDING"; break;
    case SSL_ERROR_RX_RECORD_TOO_LONG: id_str = "SSL_ERROR_RX_RECORD_TOO_LONG"; break;
    case SSL_ERROR_TX_RECORD_TOO_LONG: id_str = "SSL_ERROR_TX_RECORD_TOO_LONG"; break;
    case SSL_ERROR_RX_MALFORMED_HELLO_REQUEST: id_str = "SSL_ERROR_RX_MALFORMED_HELLO_REQUEST"; break;
    case SSL_ERROR_RX_MALFORMED_CLIENT_HELLO: id_str = "SSL_ERROR_RX_MALFORMED_CLIENT_HELLO"; break;
    case SSL_ERROR_RX_MALFORMED_SERVER_HELLO: id_str = "SSL_ERROR_RX_MALFORMED_SERVER_HELLO"; break;
    case SSL_ERROR_RX_MALFORMED_CERTIFICATE: id_str = "SSL_ERROR_RX_MALFORMED_CERTIFICATE"; break;
    case SSL_ERROR_RX_MALFORMED_SERVER_KEY_EXCH: id_str = "SSL_ERROR_RX_MALFORMED_SERVER_KEY_EXCH"; break;
    case SSL_ERROR_RX_MALFORMED_CERT_REQUEST: id_str = "SSL_ERROR_RX_MALFORMED_CERT_REQUEST"; break;
    case SSL_ERROR_RX_MALFORMED_HELLO_DONE: id_str = "SSL_ERROR_RX_MALFORMED_HELLO_DONE"; break;
    case SSL_ERROR_RX_MALFORMED_CERT_VERIFY: id_str = "SSL_ERROR_RX_MALFORMED_CERT_VERIFY"; break;
    case SSL_ERROR_RX_MALFORMED_CLIENT_KEY_EXCH: id_str = "SSL_ERROR_RX_MALFORMED_CLIENT_KEY_EXCH"; break;
    case SSL_ERROR_RX_MALFORMED_FINISHED: id_str = "SSL_ERROR_RX_MALFORMED_FINISHED"; break;
    case SSL_ERROR_RX_MALFORMED_CHANGE_CIPHER: id_str = "SSL_ERROR_RX_MALFORMED_CHANGE_CIPHER"; break;
    case SSL_ERROR_RX_MALFORMED_ALERT: id_str = "SSL_ERROR_RX_MALFORMED_ALERT"; break;
    case SSL_ERROR_RX_MALFORMED_HANDSHAKE: id_str = "SSL_ERROR_RX_MALFORMED_HANDSHAKE"; break;
    case SSL_ERROR_RX_MALFORMED_APPLICATION_DATA: id_str = "SSL_ERROR_RX_MALFORMED_APPLICATION_DATA"; break;
    case SSL_ERROR_RX_UNEXPECTED_HELLO_REQUEST: id_str = "SSL_ERROR_RX_UNEXPECTED_HELLO_REQUEST"; break;
    case SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO: id_str = "SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO"; break;
    case SSL_ERROR_RX_UNEXPECTED_SERVER_HELLO: id_str = "SSL_ERROR_RX_UNEXPECTED_SERVER_HELLO"; break;
    case SSL_ERROR_RX_UNEXPECTED_CERTIFICATE: id_str = "SSL_ERROR_RX_UNEXPECTED_CERTIFICATE"; break;
    case SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH: id_str = "SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH"; break;
    case SSL_ERROR_RX_UNEXPECTED_CERT_REQUEST: id_str = "SSL_ERROR_RX_UNEXPECTED_CERT_REQUEST"; break;
    case SSL_ERROR_RX_UNEXPECTED_HELLO_DONE: id_str = "SSL_ERROR_RX_UNEXPECTED_HELLO_DONE"; break;
    case SSL_ERROR_RX_UNEXPECTED_CERT_VERIFY: id_str = "SSL_ERROR_RX_UNEXPECTED_CERT_VERIFY"; break;
    case SSL_ERROR_RX_UNEXPECTED_CLIENT_KEY_EXCH: id_str = "SSL_ERROR_RX_UNEXPECTED_CLIENT_KEY_EXCH"; break;
    case SSL_ERROR_RX_UNEXPECTED_FINISHED: id_str = "SSL_ERROR_RX_UNEXPECTED_FINISHED"; break;
    case SSL_ERROR_RX_UNEXPECTED_CHANGE_CIPHER: id_str = "SSL_ERROR_RX_UNEXPECTED_CHANGE_CIPHER"; break;
    case SSL_ERROR_RX_UNEXPECTED_ALERT: id_str = "SSL_ERROR_RX_UNEXPECTED_ALERT"; break;
    case SSL_ERROR_RX_UNEXPECTED_HANDSHAKE: id_str = "SSL_ERROR_RX_UNEXPECTED_HANDSHAKE"; break;
    case SSL_ERROR_RX_UNEXPECTED_APPLICATION_DATA: id_str = "SSL_ERROR_RX_UNEXPECTED_APPLICATION_DATA"; break;
    case SSL_ERROR_RX_UNKNOWN_RECORD_TYPE: id_str = "SSL_ERROR_RX_UNKNOWN_RECORD_TYPE"; break;
    case SSL_ERROR_RX_UNKNOWN_HANDSHAKE: id_str = "SSL_ERROR_RX_UNKNOWN_HANDSHAKE"; break;
    case SSL_ERROR_RX_UNKNOWN_ALERT: id_str = "SSL_ERROR_RX_UNKNOWN_ALERT"; break;
    case SSL_ERROR_CLOSE_NOTIFY_ALERT: id_str = "SSL_ERROR_CLOSE_NOTIFY_ALERT"; break;
    case SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT: id_str = "SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT"; break;
    case SSL_ERROR_DECOMPRESSION_FAILURE_ALERT: id_str = "SSL_ERROR_DECOMPRESSION_FAILURE_ALERT"; break;
    case SSL_ERROR_HANDSHAKE_FAILURE_ALERT: id_str = "SSL_ERROR_HANDSHAKE_FAILURE_ALERT"; break;
    case SSL_ERROR_ILLEGAL_PARAMETER_ALERT: id_str = "SSL_ERROR_ILLEGAL_PARAMETER_ALERT"; break;
    case SSL_ERROR_UNSUPPORTED_CERT_ALERT: id_str = "SSL_ERROR_UNSUPPORTED_CERT_ALERT"; break;
    case SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT: id_str = "SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT"; break;
    case SSL_ERROR_GENERATE_RANDOM_FAILURE: id_str = "SSL_ERROR_GENERATE_RANDOM_FAILURE"; break;
    case SSL_ERROR_SIGN_HASHES_FAILURE: id_str = "SSL_ERROR_SIGN_HASHES_FAILURE"; break;
    case SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE: id_str = "SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE"; break;
    case SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE: id_str = "SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE"; break;
    case SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE: id_str = "SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE"; break;
    case SSL_ERROR_ENCRYPTION_FAILURE: id_str = "SSL_ERROR_ENCRYPTION_FAILURE"; break;
    case SSL_ERROR_DECRYPTION_FAILURE: id_str = "SSL_ERROR_DECRYPTION_FAILURE"; break;
    case SSL_ERROR_SOCKET_WRITE_FAILURE: id_str = "SSL_ERROR_SOCKET_WRITE_FAILURE"; break;
    case SSL_ERROR_MD5_DIGEST_FAILURE: id_str = "SSL_ERROR_MD5_DIGEST_FAILURE"; break;
    case SSL_ERROR_SHA_DIGEST_FAILURE: id_str = "SSL_ERROR_SHA_DIGEST_FAILURE"; break;
    case SSL_ERROR_MAC_COMPUTATION_FAILURE: id_str = "SSL_ERROR_MAC_COMPUTATION_FAILURE"; break;
    case SSL_ERROR_SYM_KEY_CONTEXT_FAILURE: id_str = "SSL_ERROR_SYM_KEY_CONTEXT_FAILURE"; break;
    case SSL_ERROR_SYM_KEY_UNWRAP_FAILURE: id_str = "SSL_ERROR_SYM_KEY_UNWRAP_FAILURE"; break;
    case SSL_ERROR_PUB_KEY_SIZE_LIMIT_EXCEEDED: id_str = "SSL_ERROR_PUB_KEY_SIZE_LIMIT_EXCEEDED"; break;
    case SSL_ERROR_IV_PARAM_FAILURE: id_str = "SSL_ERROR_IV_PARAM_FAILURE"; break;
    case SSL_ERROR_INIT_CIPHER_SUITE_FAILURE: id_str = "SSL_ERROR_INIT_CIPHER_SUITE_FAILURE"; break;
    case SSL_ERROR_SESSION_KEY_GEN_FAILURE: id_str = "SSL_ERROR_SESSION_KEY_GEN_FAILURE"; break;
    case SSL_ERROR_NO_SERVER_KEY_FOR_ALG: id_str = "SSL_ERROR_NO_SERVER_KEY_FOR_ALG"; break;
    case SSL_ERROR_TOKEN_INSERTION_REMOVAL: id_str = "SSL_ERROR_TOKEN_INSERTION_REMOVAL"; break;
    case SSL_ERROR_TOKEN_SLOT_NOT_FOUND: id_str = "SSL_ERROR_TOKEN_SLOT_NOT_FOUND"; break;
    case SSL_ERROR_NO_COMPRESSION_OVERLAP: id_str = "SSL_ERROR_NO_COMPRESSION_OVERLAP"; break;
    case SSL_ERROR_HANDSHAKE_NOT_COMPLETED: id_str = "SSL_ERROR_HANDSHAKE_NOT_COMPLETED"; break;
    case SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE: id_str = "SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE"; break;
    case SSL_ERROR_CERT_KEA_MISMATCH: id_str = "SSL_ERROR_CERT_KEA_MISMATCH"; break;
    case SSL_ERROR_NO_TRUSTED_SSL_CLIENT_CA: id_str = "SSL_ERROR_NO_TRUSTED_SSL_CLIENT_CA"; break;
    case SSL_ERROR_SESSION_NOT_FOUND: id_str = "SSL_ERROR_SESSION_NOT_FOUND"; break;
    case SSL_ERROR_DECRYPTION_FAILED_ALERT: id_str = "SSL_ERROR_DECRYPTION_FAILED_ALERT"; break;
    case SSL_ERROR_RECORD_OVERFLOW_ALERT: id_str = "SSL_ERROR_RECORD_OVERFLOW_ALERT"; break;
    case SSL_ERROR_UNKNOWN_CA_ALERT: id_str = "SSL_ERROR_UNKNOWN_CA_ALERT"; break;
    case SSL_ERROR_ACCESS_DENIED_ALERT: id_str = "SSL_ERROR_ACCESS_DENIED_ALERT"; break;
    case SSL_ERROR_DECODE_ERROR_ALERT: id_str = "SSL_ERROR_DECODE_ERROR_ALERT"; break;
    case SSL_ERROR_DECRYPT_ERROR_ALERT: id_str = "SSL_ERROR_DECRYPT_ERROR_ALERT"; break;
    case SSL_ERROR_EXPORT_RESTRICTION_ALERT: id_str = "SSL_ERROR_EXPORT_RESTRICTION_ALERT"; break;
    case SSL_ERROR_PROTOCOL_VERSION_ALERT: id_str = "SSL_ERROR_PROTOCOL_VERSION_ALERT"; break;
    case SSL_ERROR_INSUFFICIENT_SECURITY_ALERT: id_str = "SSL_ERROR_INSUFFICIENT_SECURITY_ALERT"; break;
    case SSL_ERROR_INTERNAL_ERROR_ALERT: id_str = "SSL_ERROR_INTERNAL_ERROR_ALERT"; break;
    case SSL_ERROR_USER_CANCELED_ALERT: id_str = "SSL_ERROR_USER_CANCELED_ALERT"; break;
    case SSL_ERROR_NO_RENEGOTIATION_ALERT: id_str = "SSL_ERROR_NO_RENEGOTIATION_ALERT"; break;
    case SSL_ERROR_SERVER_CACHE_NOT_CONFIGURED: id_str = "SSL_ERROR_SERVER_CACHE_NOT_CONFIGURED"; break;
    case SSL_ERROR_UNSUPPORTED_EXTENSION_ALERT: id_str = "SSL_ERROR_UNSUPPORTED_EXTENSION_ALERT"; break;
    case SSL_ERROR_CERTIFICATE_UNOBTAINABLE_ALERT: id_str = "SSL_ERROR_CERTIFICATE_UNOBTAINABLE_ALERT"; break;
    case SSL_ERROR_UNRECOGNIZED_NAME_ALERT: id_str = "SSL_ERROR_UNRECOGNIZED_NAME_ALERT"; break;
    case SSL_ERROR_BAD_CERT_STATUS_RESPONSE_ALERT: id_str = "SSL_ERROR_BAD_CERT_STATUS_RESPONSE_ALERT"; break;
    case SSL_ERROR_BAD_CERT_HASH_VALUE_ALERT: id_str = "SSL_ERROR_BAD_CERT_HASH_VALUE_ALERT"; break;
    case SSL_ERROR_RX_UNEXPECTED_NEW_SESSION_TICKET: id_str = "SSL_ERROR_RX_UNEXPECTED_NEW_SESSION_TICKET"; break;
    case SSL_ERROR_RX_MALFORMED_NEW_SESSION_TICKET: id_str = "SSL_ERROR_RX_MALFORMED_NEW_SESSION_TICKET"; break;  
    case SSL_ERROR_DECOMPRESSION_FAILURE: id_str = "SSL_ERROR_DECOMPRESSION_FAILURE"; break;                      
    case SSL_ERROR_RENEGOTIATION_NOT_ALLOWED: id_str = "SSL_ERROR_RENEGOTIATION_NOT_ALLOWED"; break;              
    case SSL_ERROR_UNSAFE_NEGOTIATION: id_str = "SSL_ERROR_UNSAFE_NEGOTIATION"; break;
    case SSL_ERROR_RX_UNEXPECTED_UNCOMPRESSED_RECORD: id_str = "SSL_ERROR_RX_UNEXPECTED_UNCOMPRESSED_RECORD"; break;
    case SSL_ERROR_WEAK_SERVER_EPHEMERAL_DH_KEY: id_str = "SSL_ERROR_WEAK_SERVER_EPHEMERAL_DH_KEY"; break;
    case SEC_ERROR_IO: id_str = "SEC_ERROR_IO"; break;
    case SEC_ERROR_LIBRARY_FAILURE: id_str = "SEC_ERROR_LIBRARY_FAILURE"; break;
    case SEC_ERROR_BAD_DATA: id_str = "SEC_ERROR_BAD_DATA"; break;
    case SEC_ERROR_OUTPUT_LEN: id_str = "SEC_ERROR_OUTPUT_LEN"; break;
    case SEC_ERROR_INPUT_LEN: id_str = "SEC_ERROR_INPUT_LEN"; break;
    case SEC_ERROR_INVALID_ARGS: id_str = "SEC_ERROR_INVALID_ARGS"; break;
    case SEC_ERROR_INVALID_ALGORITHM: id_str = "SEC_ERROR_INVALID_ALGORITHM"; break;
    case SEC_ERROR_INVALID_AVA: id_str = "SEC_ERROR_INVALID_AVA"; break;
    case SEC_ERROR_INVALID_TIME: id_str = "SEC_ERROR_INVALID_TIME"; break;
    case SEC_ERROR_BAD_DER: id_str = "SEC_ERROR_BAD_DER"; break;
    case SEC_ERROR_BAD_SIGNATURE: id_str = "SEC_ERROR_BAD_SIGNATURE"; break;
    case SEC_ERROR_EXPIRED_CERTIFICATE: id_str = "SEC_ERROR_EXPIRED_CERTIFICATE"; break;
    case SEC_ERROR_REVOKED_CERTIFICATE: id_str = "SEC_ERROR_REVOKED_CERTIFICATE"; break;
    case SEC_ERROR_UNKNOWN_ISSUER: id_str = "SEC_ERROR_UNKNOWN_ISSUER"; break;
    case SEC_ERROR_BAD_KEY: id_str = "SEC_ERROR_BAD_KEY"; break;
    case SEC_ERROR_BAD_PASSWORD: id_str = "SEC_ERROR_BAD_PASSWORD"; break;
    case SEC_ERROR_RETRY_PASSWORD: id_str = "SEC_ERROR_RETRY_PASSWORD"; break;
    case SEC_ERROR_NO_NODELOCK: id_str = "SEC_ERROR_NO_NODELOCK"; break;
    case SEC_ERROR_BAD_DATABASE: id_str = "SEC_ERROR_BAD_DATABASE"; break;
    case SEC_ERROR_NO_MEMORY: id_str = "SEC_ERROR_NO_MEMORY"; break;
    case SEC_ERROR_UNTRUSTED_ISSUER: id_str = "SEC_ERROR_UNTRUSTED_ISSUER"; break;
    case SEC_ERROR_UNTRUSTED_CERT: id_str = "SEC_ERROR_UNTRUSTED_CERT"; break;
    case SEC_ERROR_DUPLICATE_CERT: id_str = "SEC_ERROR_DUPLICATE_CERT"; break;
    case SEC_ERROR_DUPLICATE_CERT_NAME: id_str = "SEC_ERROR_DUPLICATE_CERT_NAME"; break;
    case SEC_ERROR_ADDING_CERT: id_str = "SEC_ERROR_ADDING_CERT"; break;
    case SEC_ERROR_FILING_KEY: id_str = "SEC_ERROR_FILING_KEY"; break;
    case SEC_ERROR_NO_KEY: id_str = "SEC_ERROR_NO_KEY"; break;
    case SEC_ERROR_CERT_VALID: id_str = "SEC_ERROR_CERT_VALID"; break;
    case SEC_ERROR_CERT_NOT_VALID: id_str = "SEC_ERROR_CERT_NOT_VALID"; break;
    case SEC_ERROR_CERT_NO_RESPONSE: id_str = "SEC_ERROR_CERT_NO_RESPONSE"; break;
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE: id_str = "SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE"; break;
    case SEC_ERROR_CRL_EXPIRED: id_str = "SEC_ERROR_CRL_EXPIRED"; break;
    case SEC_ERROR_CRL_BAD_SIGNATURE: id_str = "SEC_ERROR_CRL_BAD_SIGNATURE"; break;
    case SEC_ERROR_CRL_INVALID: id_str = "SEC_ERROR_CRL_INVALID"; break;
    case SEC_ERROR_EXTENSION_VALUE_INVALID: id_str = "SEC_ERROR_EXTENSION_VALUE_INVALID"; break;
    case SEC_ERROR_EXTENSION_NOT_FOUND: id_str = "SEC_ERROR_EXTENSION_NOT_FOUND"; break;
    case SEC_ERROR_CA_CERT_INVALID: id_str = "SEC_ERROR_CA_CERT_INVALID"; break;
    case SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID: id_str = "SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID"; break;
    case SEC_ERROR_CERT_USAGES_INVALID: id_str = "SEC_ERROR_CERT_USAGES_INVALID"; break;
    case SEC_INTERNAL_ONLY: id_str = "SEC_INTERNAL_ONLY"; break;
    case SEC_ERROR_INVALID_KEY: id_str = "SEC_ERROR_INVALID_KEY"; break;
    case SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION: id_str = "SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION"; break;
    case SEC_ERROR_OLD_CRL: id_str = "SEC_ERROR_OLD_CRL"; break;
    case SEC_ERROR_NO_EMAIL_CERT: id_str = "SEC_ERROR_NO_EMAIL_CERT"; break;
    case SEC_ERROR_NO_RECIPIENT_CERTS_QUERY: id_str = "SEC_ERROR_NO_RECIPIENT_CERTS_QUERY"; break;
    case SEC_ERROR_NOT_A_RECIPIENT: id_str = "SEC_ERROR_NOT_A_RECIPIENT"; break;
    case SEC_ERROR_PKCS7_KEYALG_MISMATCH: id_str = "SEC_ERROR_PKCS7_KEYALG_MISMATCH"; break;
    case SEC_ERROR_PKCS7_BAD_SIGNATURE: id_str = "SEC_ERROR_PKCS7_BAD_SIGNATURE"; break;
    case SEC_ERROR_UNSUPPORTED_KEYALG: id_str = "SEC_ERROR_UNSUPPORTED_KEYALG"; break;
    case SEC_ERROR_DECRYPTION_DISALLOWED: id_str = "SEC_ERROR_DECRYPTION_DISALLOWED"; break;
    case XP_SEC_FORTEZZA_BAD_CARD: id_str = "XP_SEC_FORTEZZA_BAD_CARD"; break;
    case XP_SEC_FORTEZZA_NO_CARD: id_str = "XP_SEC_FORTEZZA_NO_CARD"; break;
    case XP_SEC_FORTEZZA_NONE_SELECTED: id_str = "XP_SEC_FORTEZZA_NONE_SELECTED"; break;
    case XP_SEC_FORTEZZA_MORE_INFO: id_str = "XP_SEC_FORTEZZA_MORE_INFO"; break;
    case XP_SEC_FORTEZZA_PERSON_NOT_FOUND: id_str = "XP_SEC_FORTEZZA_PERSON_NOT_FOUND"; break;
    case XP_SEC_FORTEZZA_NO_MORE_INFO: id_str = "XP_SEC_FORTEZZA_NO_MORE_INFO"; break;
    case XP_SEC_FORTEZZA_BAD_PIN: id_str = "XP_SEC_FORTEZZA_BAD_PIN"; break;
    case XP_SEC_FORTEZZA_PERSON_ERROR: id_str = "XP_SEC_FORTEZZA_PERSON_ERROR"; break;
    case SEC_ERROR_NO_KRL: id_str = "SEC_ERROR_NO_KRL"; break;
    case SEC_ERROR_KRL_EXPIRED: id_str = "SEC_ERROR_KRL_EXPIRED"; break;
    case SEC_ERROR_KRL_BAD_SIGNATURE: id_str = "SEC_ERROR_KRL_BAD_SIGNATURE"; break;
    case SEC_ERROR_REVOKED_KEY: id_str = "SEC_ERROR_REVOKED_KEY"; break;
    case SEC_ERROR_KRL_INVALID: id_str = "SEC_ERROR_KRL_INVALID"; break;
    case SEC_ERROR_NEED_RANDOM: id_str = "SEC_ERROR_NEED_RANDOM"; break;
    case SEC_ERROR_NO_MODULE: id_str = "SEC_ERROR_NO_MODULE"; break;
    case SEC_ERROR_NO_TOKEN: id_str = "SEC_ERROR_NO_TOKEN"; break;
    case SEC_ERROR_READ_ONLY: id_str = "SEC_ERROR_READ_ONLY"; break;
    case SEC_ERROR_NO_SLOT_SELECTED: id_str = "SEC_ERROR_NO_SLOT_SELECTED"; break;
    case SEC_ERROR_CERT_NICKNAME_COLLISION: id_str = "SEC_ERROR_CERT_NICKNAME_COLLISION"; break;
    case SEC_ERROR_KEY_NICKNAME_COLLISION: id_str = "SEC_ERROR_KEY_NICKNAME_COLLISION"; break;
    case SEC_ERROR_SAFE_NOT_CREATED: id_str = "SEC_ERROR_SAFE_NOT_CREATED"; break;
    case SEC_ERROR_BAGGAGE_NOT_CREATED: id_str = "SEC_ERROR_BAGGAGE_NOT_CREATED"; break;
    case XP_JAVA_REMOVE_PRINCIPAL_ERROR: id_str = "XP_JAVA_REMOVE_PRINCIPAL_ERROR"; break;
    case XP_JAVA_DELETE_PRIVILEGE_ERROR: id_str = "XP_JAVA_DELETE_PRIVILEGE_ERROR"; break;
    case XP_JAVA_CERT_NOT_EXISTS_ERROR: id_str = "XP_JAVA_CERT_NOT_EXISTS_ERROR"; break;
    case SEC_ERROR_BAD_EXPORT_ALGORITHM: id_str = "SEC_ERROR_BAD_EXPORT_ALGORITHM"; break;
    case SEC_ERROR_EXPORTING_CERTIFICATES: id_str = "SEC_ERROR_EXPORTING_CERTIFICATES"; break;
    case SEC_ERROR_IMPORTING_CERTIFICATES: id_str = "SEC_ERROR_IMPORTING_CERTIFICATES"; break;
    case SEC_ERROR_PKCS12_DECODING_PFX: id_str = "SEC_ERROR_PKCS12_DECODING_PFX"; break;
    case SEC_ERROR_PKCS12_INVALID_MAC: id_str = "SEC_ERROR_PKCS12_INVALID_MAC"; break;
    case SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM: id_str = "SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM"; break;
    case SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE: id_str = "SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE"; break;
    case SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE: id_str = "SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE"; break;
    case SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM: id_str = "SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM"; break;
    case SEC_ERROR_PKCS12_UNSUPPORTED_VERSION: id_str = "SEC_ERROR_PKCS12_UNSUPPORTED_VERSION"; break;
    case SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT: id_str = "SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT"; break;
    case SEC_ERROR_PKCS12_CERT_COLLISION: id_str = "SEC_ERROR_PKCS12_CERT_COLLISION"; break;
    case SEC_ERROR_USER_CANCELLED: id_str = "SEC_ERROR_USER_CANCELLED"; break;
    case SEC_ERROR_PKCS12_DUPLICATE_DATA: id_str = "SEC_ERROR_PKCS12_DUPLICATE_DATA"; break;
    case SEC_ERROR_MESSAGE_SEND_ABORTED: id_str = "SEC_ERROR_MESSAGE_SEND_ABORTED"; break;
    case SEC_ERROR_INADEQUATE_KEY_USAGE: id_str = "SEC_ERROR_INADEQUATE_KEY_USAGE"; break;
    case SEC_ERROR_INADEQUATE_CERT_TYPE: id_str = "SEC_ERROR_INADEQUATE_CERT_TYPE"; break;
    case SEC_ERROR_CERT_ADDR_MISMATCH: id_str = "SEC_ERROR_CERT_ADDR_MISMATCH"; break;
    case SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY: id_str = "SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY"; break;
    case SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN: id_str = "SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN"; break;
    case SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME: id_str = "SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME"; break;
    case SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY: id_str = "SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY"; break;
    case SEC_ERROR_PKCS12_UNABLE_TO_WRITE: id_str = "SEC_ERROR_PKCS12_UNABLE_TO_WRITE"; break;
    case SEC_ERROR_PKCS12_UNABLE_TO_READ: id_str = "SEC_ERROR_PKCS12_UNABLE_TO_READ"; break;
    case SEC_ERROR_PKCS12_KEY_DATABASE_NOT_INITIALIZED: id_str = "SEC_ERROR_PKCS12_KEY_DATABASE_NOT_INITIALIZED"; break;
    case SEC_ERROR_KEYGEN_FAIL: id_str = "SEC_ERROR_KEYGEN_FAIL"; break;
    case SEC_ERROR_INVALID_PASSWORD: id_str = "SEC_ERROR_INVALID_PASSWORD"; break;
    case SEC_ERROR_RETRY_OLD_PASSWORD: id_str = "SEC_ERROR_RETRY_OLD_PASSWORD"; break;
    case SEC_ERROR_BAD_NICKNAME: id_str = "SEC_ERROR_BAD_NICKNAME"; break;
    case SEC_ERROR_NOT_FORTEZZA_ISSUER: id_str = "SEC_ERROR_NOT_FORTEZZA_ISSUER"; break;
    case SEC_ERROR_CANNOT_MOVE_SENSITIVE_KEY: id_str = "SEC_ERROR_CANNOT_MOVE_SENSITIVE_KEY"; break;
    case SEC_ERROR_JS_INVALID_MODULE_NAME: id_str = "SEC_ERROR_JS_INVALID_MODULE_NAME"; break;
    case SEC_ERROR_JS_INVALID_DLL: id_str = "SEC_ERROR_JS_INVALID_DLL"; break;
    case SEC_ERROR_JS_ADD_MOD_FAILURE: id_str = "SEC_ERROR_JS_ADD_MOD_FAILURE"; break;
    case SEC_ERROR_JS_DEL_MOD_FAILURE: id_str = "SEC_ERROR_JS_DEL_MOD_FAILURE"; break;
    case SEC_ERROR_OLD_KRL: id_str = "SEC_ERROR_OLD_KRL"; break;
    case SEC_ERROR_CKL_CONFLICT: id_str = "SEC_ERROR_CKL_CONFLICT"; break;
    case SEC_ERROR_CERT_NOT_IN_NAME_SPACE: id_str = "SEC_ERROR_CERT_NOT_IN_NAME_SPACE"; break;
    case SEC_ERROR_KRL_NOT_YET_VALID: id_str = "SEC_ERROR_KRL_NOT_YET_VALID"; break;
    case SEC_ERROR_CRL_NOT_YET_VALID: id_str = "SEC_ERROR_CRL_NOT_YET_VALID"; break;
    case SEC_ERROR_UNKNOWN_CERT: id_str = "SEC_ERROR_UNKNOWN_CERT"; break;
    case SEC_ERROR_UNKNOWN_SIGNER: id_str = "SEC_ERROR_UNKNOWN_SIGNER"; break;
    case SEC_ERROR_CERT_BAD_ACCESS_LOCATION: id_str = "SEC_ERROR_CERT_BAD_ACCESS_LOCATION"; break;
    case SEC_ERROR_OCSP_UNKNOWN_RESPONSE_TYPE: id_str = "SEC_ERROR_OCSP_UNKNOWN_RESPONSE_TYPE"; break;
    case SEC_ERROR_OCSP_BAD_HTTP_RESPONSE: id_str = "SEC_ERROR_OCSP_BAD_HTTP_RESPONSE"; break;
    case SEC_ERROR_OCSP_MALFORMED_REQUEST: id_str = "SEC_ERROR_OCSP_MALFORMED_REQUEST"; break;
    case SEC_ERROR_OCSP_SERVER_ERROR: id_str = "SEC_ERROR_OCSP_SERVER_ERROR"; break;
    case SEC_ERROR_OCSP_TRY_SERVER_LATER: id_str = "SEC_ERROR_OCSP_TRY_SERVER_LATER"; break;
    case SEC_ERROR_OCSP_REQUEST_NEEDS_SIG: id_str = "SEC_ERROR_OCSP_REQUEST_NEEDS_SIG"; break;
    case SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST: id_str = "SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST"; break;
    case SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS: id_str = "SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS"; break;
    case SEC_ERROR_OCSP_UNKNOWN_CERT: id_str = "SEC_ERROR_OCSP_UNKNOWN_CERT"; break;
    case SEC_ERROR_OCSP_NOT_ENABLED: id_str = "SEC_ERROR_OCSP_NOT_ENABLED"; break;
    case SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER: id_str = "SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER"; break;
    case SEC_ERROR_OCSP_MALFORMED_RESPONSE: id_str = "SEC_ERROR_OCSP_MALFORMED_RESPONSE"; break;
    case SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE: id_str = "SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE"; break;
    case SEC_ERROR_OCSP_FUTURE_RESPONSE: id_str = "SEC_ERROR_OCSP_FUTURE_RESPONSE"; break;
    case SEC_ERROR_OCSP_OLD_RESPONSE: id_str = "SEC_ERROR_OCSP_OLD_RESPONSE"; break;
    case SEC_ERROR_DIGEST_NOT_FOUND: id_str = "SEC_ERROR_DIGEST_NOT_FOUND"; break;
    case SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE: id_str = "SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE"; break;
    case SEC_ERROR_MODULE_STUCK: id_str = "SEC_ERROR_MODULE_STUCK"; break;
    case SEC_ERROR_BAD_TEMPLATE: id_str = "SEC_ERROR_BAD_TEMPLATE"; break;
    case SEC_ERROR_CRL_NOT_FOUND: id_str = "SEC_ERROR_CRL_NOT_FOUND"; break;
    case SEC_ERROR_REUSED_ISSUER_AND_SERIAL: id_str = "SEC_ERROR_REUSED_ISSUER_AND_SERIAL"; break;
    case SEC_ERROR_BUSY: id_str = "SEC_ERROR_BUSY"; break;
    case SEC_ERROR_EXTRA_INPUT: id_str = "SEC_ERROR_EXTRA_INPUT"; break;
    case SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE: id_str = "SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE"; break;
    case SEC_ERROR_UNSUPPORTED_EC_POINT_FORM: id_str = "SEC_ERROR_UNSUPPORTED_EC_POINT_FORM"; break;
    case SEC_ERROR_UNRECOGNIZED_OID: id_str = "SEC_ERROR_UNRECOGNIZED_OID"; break;
    case SEC_ERROR_OCSP_INVALID_SIGNING_CERT: id_str = "SEC_ERROR_OCSP_INVALID_SIGNING_CERT"; break;
    case SEC_ERROR_REVOKED_CERTIFICATE_CRL: id_str = "SEC_ERROR_REVOKED_CERTIFICATE_CRL"; break;
    case SEC_ERROR_REVOKED_CERTIFICATE_OCSP: id_str = "SEC_ERROR_REVOKED_CERTIFICATE_OCSP"; break;
    case SEC_ERROR_CRL_INVALID_VERSION: id_str = "SEC_ERROR_CRL_INVALID_VERSION"; break;
    case SEC_ERROR_CRL_V1_CRITICAL_EXTENSION: id_str = "SEC_ERROR_CRL_V1_CRITICAL_EXTENSION"; break;
    case SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION: id_str = "SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION"; break;
    case SEC_ERROR_UNKNOWN_OBJECT_TYPE: id_str = "SEC_ERROR_UNKNOWN_OBJECT_TYPE"; break;
    case SEC_ERROR_INCOMPATIBLE_PKCS11: id_str = "SEC_ERROR_INCOMPATIBLE_PKCS11"; break;
    case SEC_ERROR_NO_EVENT: id_str = "SEC_ERROR_NO_EVENT"; break;
    case SEC_ERROR_CRL_ALREADY_EXISTS: id_str = "SEC_ERROR_CRL_ALREADY_EXISTS"; break;
    case SEC_ERROR_NOT_INITIALIZED: id_str = "SEC_ERROR_NOT_INITIALIZED"; break;
    case SEC_ERROR_TOKEN_NOT_LOGGED_IN: id_str = "SEC_ERROR_TOKEN_NOT_LOGGED_IN"; break;
    case SEC_ERROR_OCSP_RESPONDER_CERT_INVALID: id_str = "SEC_ERROR_OCSP_RESPONDER_CERT_INVALID"; break;
    case SEC_ERROR_OCSP_BAD_SIGNATURE: id_str = "SEC_ERROR_OCSP_BAD_SIGNATURE"; break;
    case SEC_ERROR_OUT_OF_SEARCH_LIMITS: id_str = "SEC_ERROR_OUT_OF_SEARCH_LIMITS"; break;
    case SEC_ERROR_INVALID_POLICY_MAPPING: id_str = "SEC_ERROR_INVALID_POLICY_MAPPING"; break;
    case SEC_ERROR_POLICY_VALIDATION_FAILED: id_str = "SEC_ERROR_POLICY_VALIDATION_FAILED"; break;
    case SEC_ERROR_UNKNOWN_AIA_LOCATION_TYPE: id_str = "SEC_ERROR_UNKNOWN_AIA_LOCATION_TYPE"; break;
    case SEC_ERROR_BAD_HTTP_RESPONSE: id_str = "SEC_ERROR_BAD_HTTP_RESPONSE"; break;
    case SEC_ERROR_BAD_LDAP_RESPONSE: id_str = "SEC_ERROR_BAD_LDAP_RESPONSE"; break;
    case SEC_ERROR_FAILED_TO_ENCODE_DATA: id_str = "SEC_ERROR_FAILED_TO_ENCODE_DATA"; break;
    case SEC_ERROR_BAD_INFO_ACCESS_LOCATION: id_str = "SEC_ERROR_BAD_INFO_ACCESS_LOCATION"; break;
    case SEC_ERROR_LIBPKIX_INTERNAL: id_str = "SEC_ERROR_LIBPKIX_INTERNAL"; break;
    case SEC_ERROR_PKCS11_GENERAL_ERROR: id_str = "SEC_ERROR_PKCS11_GENERAL_ERROR"; break;
    case SEC_ERROR_PKCS11_FUNCTION_FAILED: id_str = "SEC_ERROR_PKCS11_FUNCTION_FAILED"; break;
    case SEC_ERROR_PKCS11_DEVICE_ERROR: id_str = "SEC_ERROR_PKCS11_DEVICE_ERROR"; break;
    case SEC_ERROR_BAD_INFO_ACCESS_METHOD: id_str = "SEC_ERROR_BAD_INFO_ACCESS_METHOD"; break;
    case SEC_ERROR_CRL_IMPORT_FAILED: id_str = "SEC_ERROR_CRL_IMPORT_FAILED"; break;
  }

  return id_str;
}

const char *
nsNSSErrors::getOverrideErrorStringName(PRInt32 aErrorCode)
{
  const char *id_str = nsnull;

  switch (aErrorCode) {
    case SSL_ERROR_SSL_DISABLED:
      id_str = "PSMERR_SSL_Disabled";
      break;
  
    case SSL_ERROR_SSL2_DISABLED:
      id_str = "PSMERR_SSL2_Disabled";
      break;
  
    case SEC_ERROR_REUSED_ISSUER_AND_SERIAL:
      id_str = "PSMERR_HostReusedIssuerSerial";
      break;
  }

  return id_str;
}

nsresult
nsNSSErrors::getErrorMessageFromCode(PRInt32 err,
                                     nsINSSComponent *component,
                                     nsString &returnedMessage)
{
  NS_ENSURE_ARG_POINTER(component);
  returnedMessage.Truncate();

  const char *nss_error_id_str = getDefaultErrorStringName(err);
  const char *id_str = getOverrideErrorStringName(err);

  if (id_str || nss_error_id_str)
  {
    nsString defMsg;
    nsresult rv;
    if (id_str)
    {
      rv = component->GetPIPNSSBundleString(id_str, defMsg);
    }
    else
    {
      rv = component->GetNSSBundleString(nss_error_id_str, defMsg);
    }

    if (NS_SUCCEEDED(rv))
    {
      returnedMessage.Append(defMsg);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }

    nsCString error_id(nss_error_id_str);
    ToLowerCase(error_id);
    NS_ConvertASCIItoUTF16 idU(error_id);

    const PRUnichar *params[1];
    params[0] = idU.get();

    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName("certErrorCodePrefix", 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
      returnedMessage.Append(formattedString);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }
    else {
      returnedMessage.Append(NS_LITERAL_STRING("("));
      returnedMessage.Append(idU);
      returnedMessage.Append(NS_LITERAL_STRING(")"));
    }
  }

  return NS_OK;
}
