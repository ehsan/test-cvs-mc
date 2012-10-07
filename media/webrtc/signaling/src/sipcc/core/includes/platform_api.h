/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
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

#ifndef _PLATFORM_API_H_
#define _PLATFORM_API_H_

#include "cpr_types.h"
#include "cpr_socket.h"
#include "ccsip_pmh.h"
#include "plat_api.h"
#include "sessionTypes.h"

void     platform_get_wired_mac_address(uint8_t *addr);
void     platform_get_active_mac_address(uint8_t *addr);
void platform_get_ip_address(cpr_ip_addr_t *ip_addr);
cpr_ip_mode_e platform_get_ip_address_mode(void);
void platform_apply_config (char * configVersionStamp, char * dialplanVersionStamp, char * fcpVersionStamp, char * cucmResult, char * loadId, char * inactiveLoadId, char * loadServer, char * logServer, boolean ppid);

/**
 * Set ip address mode
 * e.g. 
 */
cpr_ip_mode_e platGetIpAddressMode(); 

/**
 * @brief Given a msg buffer, returns a pointer to the buffer's header
 *
 * The cprGetSysHeader function retrieves the system header buffer for the
 * passed in message buffer.
 *
 * @param[in] buffer  pointer to the buffer whose sysHdr to return
 *
 * @return        Abstract pointer to the msg buffer's system header
 *                or #NULL if failure
 */
void *
cprGetSysHeader (void *buffer);

/**
 * @brief Called when the application is done with this system header
 *
 * The cprReleaseSysHeader function returns the system header buffer to the
 * system.
 * @param[in] syshdr  pointer to the sysHdr to be released
 *
 * @return        none
 */
void
cprReleaseSysHeader (void *syshdr);

#endif
