<?php
// $Id$

/**
 * @file
 * Contains Drupal form API callbacks.
 */

/**
 * Form callback for module settings.
 *
 * @ingroup forms
 * @see system_settings_form()
 */
function inbound_checker_form_settings() {
  $form = array();

  $form['inbound_checker_keyword'] = array(
    '#type' => 'textfield',
    '#title' => t('Keyword'),
    '#description' => t('The keyword to seek in external sites. e.g.‌ <em>www.example.com</em>'),
    '#default_value' => variable_get('inbound_checker_keyword', ''),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}

/**
 * Form callback for site addition page.
 *
 * @ingroup forms
 */
function inbound_checker_form_add() {
  $form = array();

  $form['inbound_checker_site_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Website name'),
    '#description' => t('Name of the external website you wish to check.'),
    '#required' => TRUE,
  );

  $form['inbound_checker_site_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Website address'),
    '#description' => t('The external site URL to seek the keyword in. e.g.:‌ http://www.example.com'),
    '#required' => TRUE,
  );

  // TODO: Remove past years from the dropdown.
  $form['inbound_checker_site_expiry'] = array(
    '#type' => 'date',
    '#title' => t('Expiry date'),
    '#description' => t('If this link is inbound ad link you can input expiry date for notify\'s you when the ad was expire.'),
  );

  $form['op'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );

  return $form;
}

/**
 * Validation callback for site add form.
 */
function inbound_checker_form_add_validate($form, &$form_state) {
  // TODO: Check the logic. Assigned to @sepehr.

  if ($form_state['values']['inbound_checker_site_name']) {
    $pos = strpos($form_state['values']['inbound_checker_site_name'], '<');
    if (!empty($pos)) {
      form_set_error($form_state['values']['inbound_checker_site_name'], t('Please enter a valid name.'));
    }
  }

  if (isset($form_state['values']['inbound_checker_site_address'])) {
    if (!preg_match('|^http(s)?://[a-z0-9-]+(.[a-z0-9-]+)*(:[0-9]+)?(/.*)?$|i', $form_state['values']['inbound_checker_site_address'])) {
        form_set_error('inbound_checker_site_address', t('Please enter a valid URL.'));
    }
  }

  // TODO: Define the module API functions and do such operations through that layer.
  $results = db_query("SELECT * FROM {inbound_checker} WHERE site_address = '%s'", $form_state['values']['inbound_checker_site_address']);
  $result = db_fetch_object($results);
  if ($result) {
    form_set_error('inbound_checker_site_address', t('Your url already existes.'));
  }
}

/**
 * Submission callback for site add form.
 */
function inbound_checker_form_add_submit($form, &$form_state) {
  $link = new stdClass();
  $link->site_name = $form_state['values']['inbound_checker_site_name'];
  $link->site_address = $form_state['values']['inbound_checker_site_address'];

  // TODO: Define a helper function to convert date element value into its timestamp.
  // TODO: Code format.
  $link->expiry = mktime(date('H'), date('i'), date('s'), $form_state['values']['inbound_checker_site_expiry']['month'], $form_state['values']['inbound_checker_site_expiry']['day'], $form_state['values']['inbound_checker_site_expiry']['year']);
  _inbound_checker_site_save($link);
  drupal_set_message(t('Website entry has been successfully created.'));
}

/**
 * Form callback sites list page.
 */
function inbound_checker_form_list() {
  // TODO: Move to forms section.

  $form = array();
  $form['#prefix'] = _inbound_checker_sites_table();
  $form['inbound_checker_check_zeros'] = array(
    '#type' => 'checkbox',
    '#title' => t('Refresh zero links.'),
    '#description' => 'Check only 0 link numbers.',
    '#default_value' => 1,
    '#return_value' => 1,
  );

  $form['ops'] = array(
    '#type' => 'submit',
    '#value' => t('Calculate Again'),
  );

  return $form;
}

/**
 * Submission callback for sites list form.
 */
function inbound_checker_form_list_submit($form, &$form_state) {
  // TODO: Check the logic. Assigned to @sepehr.
  // TODO: Complete the $batch array.

  $results = db_query("SELECT * FROM {inbound_checker}");
  $batch = array(
    'title' => t('get links...'),
    'operations' => array(),
    'finished' => 'my_finished_callback', // TODO: Remove!!!
  );

  while ($result = db_fetch_object($results)) {
    // TODO: Check $form_state['values'] instead.
    if (($result->link_num == 0 && $form_state['clicked_button']['#post']['inbound_checker_check_zeros']) || (!$form_state['clicked_button']['#post']['inbound_checker_check_zeros'])) {
      $batch['operations'][] = array('_inbound_checker_get_links_with_batch', array($result)); //TODO: Rename batch helper.
    }
  }
  batch_set($batch);

}

