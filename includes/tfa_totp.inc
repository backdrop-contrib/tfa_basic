<?php

/**
 * @file classes for tfa_totp
 */

/**
 * Class TfaTotp
 */
class TfaTotp extends TfaBasePlugin implements TfaValidationPluginInterface {

  /**
   * @var SimpleAuthenticator
   */
  protected $sa;

  /**
   * @var int
   */
  protected $timeSkew;

  /**
   * @var bool
   */
  protected $alreadyAccepted;

  /**
   * @copydoc TfaBasePlugin::__construct()
   */
  public function __construct(array $context) {
    parent::__construct($context);
    $this->sa = new \SebastianDevs\SimpleAuthenticator(6, 'SHA1');
    // Allow codes within tolerance range of 3 * 30 second units.
    $this->timeSkew = config_get('tfa_basic.settings', 'tfa_basic_time_skew');
    // Recommended: set variable tfa_totp_secret_key in settings.php.
    $this->encryptionKey = settings_get('tfa_basic_secret_key', backdrop_get_private_key());
    $this->alreadyAccepted = FALSE;
  }

  /**
   * @copydoc TfaBasePlugin::ready()
   */
  public function ready() {
    return ($this->getSeed() !== FALSE);
  }

  /**
   * @copydoc TfaValidationPluginInterface::getForm()
   */
  public function getForm(array $form, array &$form_state) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Application verification code'),
      '#description' => t('Verification code is application generated and !length digits long.', array('!length' => $this->codeLength)),
      '#required' => TRUE,
      '#attributes' => array('autocomplete' => 'off'),
    );
    if (module_exists('elements')) {
      $form['code']['#type'] = 'numberfield';
    }
    $form['actions']['#type'] = 'actions';
    $form['actions']['login'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );

    return $form;
  }

  /**
   * @copydoc TfaValidationPluginInterface::validateForm()
   */
  public function validateForm(array $form, array &$form_state) {
    if (!$this->validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid application code. Please try again.');
      if ($this->alreadyAccepted) {
        $this->errorMessages['code'] = t('Invalid code, it was recently used for a login. Please wait for the application to generate a new code.');
      }
      return FALSE;
    }
    else {
      // Store accepted code to prevent replay attacks.
      $this->storeAcceptedCode($form_state['values']['code']);
      return TRUE;
    }
  }

  /**
   * @copydoc TfaBasePlugin::validate()
   */
  protected function validate($code) {
    // Strip whitespace.
    $code = preg_replace('/\s+/', '', $code);
    if ($this->alreadyAcceptedCode($code)) {
      $this->isValid = FALSE;
    }
    else {
      // Get OTP seed.
      $seed = $this->getSeed();
      $this->isValid = ($seed && $this->sa->verifyCode($seed, $code, $this->timeSkew));
    }
    return $this->isValid;
  }

  /**
   * @param string $code
   */
  protected function storeAcceptedCode($code) {
    $code = preg_replace('/\s+/', '', $code);
    $hash = hash('sha1', backdrop_get_hash_salt() . $code);
    db_insert('tfa_accepted_code')
      ->fields(array(
        'uid' => $this->context['uid'],
        'code_hash' => $hash,
        'time_accepted' => REQUEST_TIME,
      ))
      ->execute();
  }

  /**
   * Whether code has recently been accepted.
   *
   * @param string $code
   * @return bool
   */
  protected function alreadyAcceptedCode($code) {
    $hash = hash('sha1', backdrop_get_hash_salt() . $code);
    $result = db_query(
      "SELECT code_hash FROM {tfa_accepted_code} WHERE uid = :uid AND code_hash = :code",
      array(':uid' => $this->context['uid'], ':code' => $hash)
    )->fetchAssoc();
    if (!empty($result)) {
      $this->alreadyAccepted = TRUE;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Get seed for this account.
   *
   * @return string Decrypted account OTP seed or FALSE if none exists.
   */
  protected function getSeed() {
    // Lookup seed for account and decrypt.
    $result = db_query("SELECT seed FROM {tfa_totp_seed} WHERE uid = :uid", array(':uid' => $this->context['uid']))->fetchAssoc();
    if (!empty($result)) {
      $encrypted = base64_decode($result['seed']);
      $seed = $this->decrypt($encrypted);
      if (!empty($seed)) {
        return $seed;
      }
    }
    return FALSE;
  }

  /**
   * Delete users seeds.
   *
   * @return int
   */
  public function deleteSeed() {
    $query = db_delete('tfa_totp_seed')
      ->condition('uid', $this->context['uid']);

    return $query->execute();
  }
}

/**
 * Class TfaTotpSetup
 */
class TfaTotpSetup extends TfaTotp implements TfaSetupPluginInterface {

  /**
   * @var string Un-encrypted seed.
   */
  protected $seed;

  /**
   * @var string
   */
  protected $namePrefix;

  /**
   * @copydoc TfaBasePlugin::__construct()
   */
  public function __construct(array $context) {
    parent::__construct($context);
    // Generate seed.
    $this->seed = $this->createSeed();
    $name_prefix = config_get('tfa_basic.settings', 'tfa_basic_name_prefix');
    $this->namePrefix = !empty($name_prefix) ? $name_prefix : config_get('system.core', 'site_name');
  }

  /**
   * @copydoc TfaSetupPluginInterface::getSetupForm()
   */
  public function getSetupForm(array $form, array &$form_state) {
    $items = array(
      l('Google Authenticator (Android, iPhone)', 'https://support.google.com/accounts/answer/1066447?hl=en', array('attributes' => array('target'=>'_blank'))),
      l('Password app (iPhone)', 'https://support.apple.com/en-us/120758', array('attributes' => array('target'=>'_blank'))),
      l('FreeOTP (Android)', 'https://play.google.com/store/apps/details?id=org.fedorahosted.freeotp', array('attributes' => array('target'=>'_blank'))),
      l('Authy (Android, iPhone)', 'https://www.authy.com', array('attributes' => array('target'=>'_blank'))),
      l('GAuth Authenticator (desktop, mobile)', 'https://gauth.apps.gbraad.nl', array('attributes' => array('target'=>'_blank'))),
      l('Bitwarden (desktop, mobile)', 'https://bitwarden.com', array('attributes' => array('target'=>'_blank'))),
      l('KeePassX (Windows, macOS, Linux)', 'https://keepassxc.org', array('attributes' => array('target'=>'_blank'))),
    );
    $form['apps'] = array(
      '#type' => 'markup',
      '#markup' => theme('item_list', array('items' => $items, 'title' => t('Install authentication code/password manager application on your mobile or desktop device:'))),
    );
    $form['info'] = array(
      '#type' => 'markup',
      '#markup' => t('<p>The two-factor authentication/password manager application will be used during this setup and for generating codes during regular authentication. If the application supports it, scan the QR code below to get the setup code otherwise you can manually enter the text code.</p>'),
    );
    $form['seed'] = array(
      '#type' => 'textfield',
      '#value' => $this->seed,
      '#disabled' => TRUE,
      '#allow_focus' => TRUE,
      '#description' => t('Enter this code into your two-factor authentication/password manager app or scan the QR code below.'),
    );
    // QR image of seed.
    $form['qr_image_wrapper']['qr_image'] = array(
      '#markup' => '<div id="tfa-qrcode"></div>',
    );
    $qrdata = 'otpauth://totp/' . $this->accountName() . '?secret=' . $this->seed;
    $form['qr_image_wrapper']['qr_image']['#attached']['library'][] = array('tfa_basic', 'qrcodejs');
    $form['qr_image_wrapper']['qr_image']['#attached']['js'][] = array(
      'data' => 'jQuery(document).ready(function () { new QRCode(document.getElementById("tfa-qrcode"), "' . $qrdata . '");});',
      'type' => 'inline',
      'scope' => 'footer',
      'weight' => 5,
    );
    // Include code entry form.
    $form = $this->getForm($form, $form_state);
    $form['actions']['login']['#value'] = t('Verify and save');
    // Alter code description.
    $form['code']['#description'] = t('A verification code will be generated after you scan the above QR code or manually enter the setup code. The verification code is six digits long.');
    return $form;
  }

  /**
   * @copydoc TfaSetupPluginInterface::validateSetupForm()
   */
  public function validateSetupForm(array $form, array &$form_state) {
    if (!$this->validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid application code. Please try again.');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  /**
   * @copydoc TfaBasePlugin::validate()
   */
  protected function validate($code) {
    return $this->sa->verifyCode($this->seed, $code, $this->timeSkew);
  }

  /**
   * @copydoc TfaSetupPluginInterface::submitSetupForm()
   */
  public function submitSetupForm(array $form, array &$form_state) {
    // Write seed for user.
    $this->storeSeed($this->seed);
    return TRUE;
  }

  /**
   * Create OTP seed for account.
   *
   * @return string Seed.
   */
  protected function createSeed() {
    return $this->sa->createSecret(24);
  }

  /**
   * Save seed for account.
   *
   * @param string $seed Seed.
   */
  protected function storeSeed($seed) {
    // Encrypt seed for storage.
    $encrypted = $this->encrypt($seed);
    // Data is binary so store base64 encoded.
    $record = array(
      'uid' => $this->context['uid'],
      'seed' => base64_encode($encrypted),
      'created' => REQUEST_TIME
    );

    $existing = $this->getSeed();
    if (!empty($existing)) {
      // Update existing seed.
      backdrop_write_record('tfa_totp_seed', $record, 'uid');
    }
    else {
      backdrop_write_record('tfa_totp_seed', $record);
    }
  }

  /**
   * Get account name for QR image.
   *
   * @return string URL encoded string.
   */
  protected function accountName() {
    $account = user_load($this->context['uid']);
    return urlencode($this->namePrefix . '-' . $account->name);
  }

}
