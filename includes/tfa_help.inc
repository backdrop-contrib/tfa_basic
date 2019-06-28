<?php

/**
 * @file class for TFA Basic
 */

/**
 * Class TfaBasicHelp
 */
class TfaBasicHelp extends TfaBasePlugin implements TfaValidationPluginInterface {

  /**
   * @copydoc TfaBasePlugin::getForm()
   */
  public function getForm(array $form, array &$form_state) {
    $content = config_get_translated('tfa_basic_help_text');
    $form['help'] = array(
      '#type' => 'markup',
      '#markup' => check_plain($content),
    );
    // Disallow login plugins from applying to this step.
    $form['#tfa_no_login'] = TRUE;
    return $form;
  }

  /**
   * @copydoc TfaValidationPluginInterface::validateForm()
   */
  public function validateForm(array $form, array &$form_state) {
    // Unused.
  }

}
