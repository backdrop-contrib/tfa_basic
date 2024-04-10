<?php

class TfaBasicEmail extends TfaBasePlugin implements TfaValidationPluginInterface, TfaSendPluginInterface {

  protected $account;

  protected $sendingEmail;

  protected $email;

  protected $messageText;

  public function __construct(array $context) {
    parent::__construct($context);
    if (!empty($context['validate_context']) && !empty($context['validate_context']['code'])) {
      $this->code = $context['validate_context']['code'];
    }

    $account = user_load($context['uid']);
    $config = config('system.core');
    $site_mail = $config->get('site_mail');
    if (empty($site_mail)) {
      $site_mail = ini_get('sendmail_from');
    }

    $this->sendingEmail = $site_mail;
    $this->codeLength = 6;
    $this->messageText = config_get_translated('tfa_basic.settings', 'tfa_basic_email_message_text');

    $this->email = $account->mail;
    if (!empty($context['email'])) {
      $this->email = $context['email'];
    }

    if (!empty($context['uid'])) {
      $this->account = user_load($context['uid']);
    }
  }

  /**
   * {inheritdoc}
   */
  public function begin() {
    if (!$this->code) {
      $this->code = $this->generate();
      if (!$this->sendCode($this->code)) {
        backdrop_set_message(t('Unable to email the code. Please contact support.'), 'error');
      }
    }
  }

  public function getForm(array $form, array &$form_state) {
    $form['code'] = array(
      '#type' => 'number',
      '#title' => t('Verification Code'),
      '#required' => TRUE,
      '#description' => t('Enter @length-character code sent to your email.', array('@length' => $this->codeLength)),
    );
    $form['actions']['#type'] = 'actions';
    // @todo optionally report on when code was sent.
    $form['actions']['login'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );
    $form['actions']['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend'),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );

    return $form;
  }

  public function validateForm(array $form, array &$form_state) {
    // If operation is resend then do not attempt to validate code.
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      return TRUE;
    }
    elseif (!parent::validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid code.');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  public function submitForm(array $form, array &$form_state) {
    // Resend code if pushed.
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      $this->code = $this->generate();
      if (!$this->sendCode($this->code)) {
        backdrop_set_message(t('Unable to email the code. Please contact support.'), 'error');
      }
      else {
        backdrop_set_message(t('Code resent.'));
      }
      return FALSE;
    }
    else {
      return parent::submitForm($form, $form_state);
    }
  }

  /**
   * Return context for this plugin.
   *
   * @return array
   */
  public function getPluginContext() {
    return array(
      'code' => $this->code,
    );
  }

  protected function generate() {
    $allowable_characters = '0123456789';

    // Zero-based count of characters in the allowable list:
    $len = strlen($allowable_characters) - 1;

    // Declare the password as a blank string.
    $string = '';

    // Loop the number of times specified by codeLength.
    for ($i = 0; $i < $this->codeLength; $i++) {
      do {
        // Find a secure random number within the range needed.
        $index = ord(backdrop_random_bytes(1));
      } while ($index > $len);

      // Each iteration, pick a random character from the
      // allowable string and append it to the password:
      $string .= $allowable_characters[$index];
    }
    return $string;
  }

  protected function getEmail() {
    return $this->email;
  }

  /**
   * Send the code via the client.
   *
   * @param string $code
   * @return bool
   */
  protected function sendCode($code) {
    global $language;
    $to = $this->getEmail();
    try {
      $params['account'] = $this->account;
      $params['code'] = $code;
      $mail = backdrop_mail('tfa_basic', 'tfa_basic_email', $to, $language, $params);
      watchdog('tfa_basic', 'Email sent to user !uid on @sent', array(
        '@sent' => $mail['result'],
        '!uid' => $this->context['uid'],
      ), WATCHDOG_INFO);
      return TRUE;
    }
    catch (\Exception $e) {
      watchdog('tfa_basic', 'Email message error to user !uid. Status code: @code, message: @message', array(
        '!uid' => $this->context['uid'],
        '@code' => $e->getCode(),
        '@message' => $e->getMessage(),
        ), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * {@inheritdoc}
   */
  public function ready() {
    $tfa_data = tfa_basic_get_tfa_data($this->account);
    // We can only send a code if there is an email.
    if (!empty($this->email) && !empty($tfa_data['data']['email'])) {
      return TRUE;
    }
    return FALSE;
  }

}

/**
 * Class TfaBasicEmailSetup
 */
class TfaBasicEmailSetup extends TfaBasicEmail implements TfaSetupPluginInterface {

  public function __construct(array $context) {
    parent::__construct($context);
  }

  public function begin() {
    if (empty($this->code)) {
      $this->code = $this->generate();
      if (!$this->sendCode($this->code)) {
        // @todo decide on error text
        $this->errorMessages[''] = t('Unable to email code to your email address.');
      }
    }
  }

  /**
   * @copydoc TfaSetupPluginInterface::getSetupForm()
   */
  public function getSetupForm(array $form, array &$form_state) {
    $tfa_data = tfa_basic_get_tfa_data($this->account);

    if (!empty($tfa_data['data']['email'])) {
      // Provide disable Email option.
      $form['actions']['email_disable'] = array(
        '#type' => 'submit',
        '#value' => t('Disable Email delivery'),
      );
    }
    else {
      $this->code = $this->generate();
      if (!$this->sendCode($this->code)) {
        backdrop_set_message(t('Unable to email the code. Please contact support.'), 'error');
      }
      else {
        backdrop_set_message(t('A code was sent to @email. It may take a minute for its arrival.', array('@email' => $this->account->mail)));
      }
      $form['email_code'] = array(
        '#type' => 'number',
        '#title' => t('Verification Code'),
        '#required' => TRUE,
        '#description' => t('Enter @length-character code sent to your email.', array('@length' => $this->codeLength)),
      );
      $form['actions']['verify'] = array(
        '#type' => 'submit',
        '#value' => t('Verify and save'),
      );
    }

    if (isset($form_state['storage']['full_setup'])) {
      backdrop_set_message(t("If the code does not arrive skip this step to continue without Email delivery. You can enable it after completing the rest of TFA setup."));
    }
    elseif(empty($tfa_data['data']['email'])) {
      $form['email_code']['#description'] .= ' ' . l(t('If the code does not arrive click here to start over.'), 'user/' . $this->account->uid . '/security/tfa/email-setup');
    }

    return $form;
  }

  /**
   * @copydoc TfaSetupPluginInterface::validateSetupForm()
   */
  public function validateSetupForm(array $form, array &$form_state) {
    // Verify the code.
    if (isset($form_state['values']['email_code'])) {
      if (!$this->validate($form_state['values']['email_code'])) {
        $this->errorMessages['email_code'] = t('Invalid code. Please try again.');
        return FALSE;
      }
    }
    // Just disable it.
    return TRUE;
  }

  /**
   * @copydoc TfaSetupPluginInterface::submitSetupForm()
   */
  public function submitSetupForm(array $form, array &$form_state) {
    // Disable Email delivery.
    if (isset($form_state['values']['email_disable']) && $form_state['values']['op'] === $form_state['values']['email_disable']) {
      tfa_basic_setup_save_data($this->account, array('email' => FALSE));
      backdrop_set_message(t('TFA Email delivery disabled.'));
      $form_state['redirect'] = 'user/' . $this->account->uid . '/security/tfa';
      watchdog('tfa_basic', 'TFA Email disabled for user @name UID !uid', array(
        '@name' => $this->account->name,
        '!uid' => $this->account->uid,
      ), WATCHDOG_INFO);
    }
    else {
      tfa_basic_setup_save_data($this->account, array('email' => TRUE));
    }
    return TRUE;
  }

}
