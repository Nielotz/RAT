#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <cstdio>
#include <iostream>

#include "serial/serial.h"

constexpr auto path = "/dev/serial/by-id/usb-WEMOS.CC_LOLIN-S2-MINI_0-if00";

constexpr pam_conv conv = {
    misc_conv, // Defined in pam_misc.h
    nullptr
};


int main() {
    using std::cout;
    using std::endl;

    pam_handle_t *pamh = nullptr;
    cout << "Initializing PAM transaction..." << endl;

    std::string username;
    cout << "Username: ";
    std::cin >> username;

    int retval = pam_start("sudo", username.c_str(), &conv, &pamh);

    // Are the credentials correct?
    if (retval == PAM_SUCCESS) {
        cout << "Initialized PAM successfully." << endl;
        retval = pam_authenticate(pamh, 0);
    }

    // Can the accound be used at this time?
    if (retval == PAM_SUCCESS) {
        cout << "Account is valid." << endl;
        retval = pam_acct_mgmt(pamh, 0);
    }

    if (retval == PAM_SUCCESS) {
        cout << "Set cred..." << endl;
        pam_setcred(pamh, PAM_ESTABLISH_CRED);
    }

    // Did everything work?
    if (retval == PAM_SUCCESS) {
        cout << "OK" << endl;
    } else {
        cout << "Not OK" << endl;
    }

    // close PAM (end session)
    if (pam_end(pamh, retval) != PAM_SUCCESS) {
        pamh = nullptr;
        cout << "check_user: failed to release authenticator" << endl;
        exit(1);
    }

    return retval == PAM_SUCCESS ? 0 : 1;
}
