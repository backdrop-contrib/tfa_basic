# Basic plugins for TFA module

Intent is to provide basic functionality of TFA Backdrop module and to be an
example of TFA plugin development.

Please use the public issue queue for all feature and support requests:
https://github.com/backdrop-contrib/tfa_basic/issues/

## Plugins

* TOTP
 A Time-based One Time Password plugin using PHP_Gansta\GoogleAuthenticator
 PHP library.

* Trusted Browsers
 A TFA login plugin allowing browsers to be marked "trusted" so that subsequent
 logins will not require TFA for a 30 day window.

* Recovery Codes
 Pre-generated one-time-use codes.

* Twilio SMS
 Optional plugin for sending TFA codes via SMS messages. See SMS section below.

## Variables

* `tfa_basic_secret_key`
 Secret key to to use as encryption key for TOTP seed encryption. Should be set
 in settings file. For example:

  `$settings['tfa_basic_secret_key'] = '1234567890';`

* `tfa_basic_time_skew`
 Number of 30 second chunks to allow TOTP keys between.

* `tfa_basic_name_prefix`
 Prefix for TOTP QR code names. Suffix is account username.

* `tfa_basic_cookie_name`
 Cookie name of TFA trusted browser cookie. Default is "TB". Rarely needs to be
 changed but can set in settings file:

 `$settings['tfa_basic_cookie_name'] = 'mycookie';`

* `tfa_basic_cookie_domain`
 Cookie domain for TFA trusted browser cookie.

* `tfa_basic_trust_cookie_expiration`
 How long before TFA cookies expire. Default is 30 days.

* `tfa_basic_accepted_code_expiration`
 How long before accepted TOTP codes expire. Default is 1 day.

## Using qrcode.js library instead of Google images

By default the module uses Google's Chart API to create a QR code. That request
will leak the seed to google in the URL of an HTTP GET request which reduces
the security of the seed. The module also supports the qrcode.js project which
will create the QR code image without leaking information to third-party sites.

To enable qrcode.js you simply have to place the library in the
tfa_basic/includes directory. From the command line:

  `cd tfa_basic/includes/`
  `git clone https://github.com/davidshimjs/qrcodejs.git`

The qrcode.min.js file should be at tfa_basic/includes/qrcodejs/qrcode.min.js

No additional setup is necessary, if the file exists in the right location then
it will be used.

## Using SMS for TFA codes

*Currently disabled until https://www.drupal.org/project/tfa_basic/issues/2997261
is resolved.*

Prerequisites:

 1. Set up a Twilio account and credit at twilio.com.
 2. Install Twilio PHP library.
 3. Provide account mobile phone number by:
   a. Creating an account textfield and configuring it for TFA use, or
   b. Implementing hook_tfa_basic_get_mobile_number_alter()

### Installing Twilio PHP library

TFA SMS plugin requires the Twilio PHP library for sending SMS codes. You can
install the Twilio PHP library manually or via the Backdrop Libraries API.

Option 1, install Twilio PHP manually:

  cd tfa_basic/includes/
  git clone https://github.com/twilio/twilio-php.git

Such that the file tfa_basic/includes/twilio-php/Services/Twilio.php exists.

Option 2, install via Backdrop Libraries API:

1. Install Backdrop Libraries API: https://www.backdropcms.org/project/libraries
2. Download the Twilio PHP library from (http://www.twilio.com/docs/libraries).
3. Extract the library in your `/libraries` folder and rename the
    directory to 'twilio'.

### Account mobile phone numbers

Accounts using SMS for TFA code delivery must have a mobile phone number able
set. By default, TFA Basic's SMS plugin has support for storing mobile numbers
in user account fields. Create a text field on an account and you can set its
use by the plugin on the TFA administration configuration page.

If you want to store the mobile number somewhere else you'll need to write a bit
of code to integrate with TFA Basic.

First, set the variable tfa_basic_phone_field to FALSE. This will inform TFA
Basic that you are using custom storage.

  `drush vset tfa_basic_phone_field FALSE`

Finally, implement hook_tfa_basic_get_mobile_number_alter() in a custom module.
The sole argument is an array with elements 'account' and 'number'. 'account' is
the Backdrop user account object you can use in finding the mobile number.
'number' will be an empty string if there's no account field in use. You should
set the 'number' property to the mobile number you have stored.

When an account is enabling SMS delivery they have the option to change the
mobile number receiving SMS codes. If the number is changed you can implement
hook_tfa_basic_set_mobile_number_alter() in a custom module to update your
storage.

#### Handling numbers that are not NANP

* Implement `hook_tfa_basic_valid_number_alter()` for number validation
* Implement `hook_tfa_basic_format_number_alter()` for formatting number output

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

## Current Maintainers

Herb v/d Dool (https://github.com/herbdool/)

This module is currently seeking co-maintainers.

## Credits

Ported to Backdrop by Herb v/d Dool (https://github.com/herbdool/)

This module was originally written for Drupal (https://drupal.org/project/tfa). Drupal maintainers are: [coltrane](https://www.drupal.org/u/coltrane).