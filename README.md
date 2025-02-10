# Basic plugins for TFA module

Intent is to provide basic functionality of TFA Backdrop module and to be an
example of TFA plugin development.

Please use the [public issue queue](https://github.com/backdrop-contrib/tfa_basic/issues/) for all feature and support requests.

## Plugins

* TOTP
 A Time-based One Time Password plugin using PHP_Gansta\GoogleAuthenticator
 PHP library.

* Trusted Browsers
 A TFA login plugin allowing browsers to be marked "trusted" so that subsequent
 logins will not require TFA for a 30 day window.

* Recovery Codes
 Pre-generated one-time-use codes.

* Email delivery
 Optional plugin for sending TFA codes via email.

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

* `tfa_basic_validation_skip`
 How many times a user can skip setting up TFA before they can no longer log in. Default is 3 times.

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

## Current Maintainers

[Herb v/d Dool](https://github.com/herbdool/)

This module is currently seeking co-maintainers.

## Credits

Ported to Backdrop by [Herb v/d Dool](https://github.com/herbdool/)

This module was originally written for [Drupal](https://drupal.org/project/tfa). Drupal maintainers are: [coltrane](https://www.drupal.org/u/coltrane).
