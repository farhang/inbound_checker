<?php
// $Id$

/**
 * @file
 * Contains form definition, validation and submission callbacks.
 */

/**
 * Form callback for module settings.
 *
 * @ingroup forms
 * @see system_settings_form()
 */
function inbound_checker_settings() {
  $form = array();

  $form['inbound_checker_keyword'] = array(
    '#type' => 'textfield',
    '#title' => t('keyword'),
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
function inbound_checker_form_site_add() {
  $form = array();

  // TODO: Prepend the module namespace to field names.

  $form['website_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Website name'),
    '#required' => TRUE,
  );

  $form['website_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Website address'),
    '#description' => t('The external site URL to seek the keyword in. e.g.:‌ http://www.example.com'),
    '#required' => TRUE,
  );

  $form['ad_date_expire'] = array(
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
function inbound_checker_form_site_add_validate($form, &$form_state) {
  // TODO: Update field names once they've been changed in form definition callback.
  // TODO: Check the logic.

  $site_address = $form_state['values']['website_address'];
  if ($form_state['values']['website_name']) {
    $pos = strpos($form_state['values']['website_name'], '<');
    if (!empty($pos)) {
      form_set_error($form_state['values']['website_name'], t('Please enter the valid name.'));
    }
  }

  if (isset($site_address)) {
    if (!preg_match('|^http(s)?://[a-z0-9-]+(.[a-z0-9-]+)*(:[0-9]+)?(/.*)?$|i', $site_address)) {
        form_set_error($site_address, t('Please enter the valid url.'));
    }
  }

  $results = db_query("SELECT * FROM {inbound_checker} WHERE site_address = '%s'", array($site_address));
  $result = db_fetch_object($results);
  if ($result) {
    form_set_error($site_address, t('Your url already existes.'));
  }
}

/**
 * Submission callback for site add form.
 */
function inbound_checker_form_site_add_submit($form, &$form_state) {
  // TODO: Update field names once they've been changed in form definition callback.

  $link = new stdClass();
  $link->site_name = $form_state['values']['website_name'];
  $link->site_address = $form_state['values']['website_address'];
  $link->expiry = mktime(date('H'), date('i'), date('s'), $form_state['values']['ad_date_expire']['month'], $form_state['values']['ad_date_expire']['day'], $form_state['values']['ad_date_expire']['year']);
  _inbound_checker_site_save($link);
  drupal_set_message(t('Entry has been created.'));
}

/**
 * Form callback sites list page.
 */
function inbound_checker_form_list() {
  // TODO: Move to forms section.
  // TODO: Prepend field names with module namespace.

  $form = array();
  $form['#prefix'] = _inbound_checker_sites_table();
  $form['only-new'] = array(
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
  // TODO: Check the logic.

  $results = db_query("SELECT * FROM {inbound_checker}");
  $batch = array(
    'title' => t('get links...'),
    'operations' => array(),
    'finished' => 'my_finished_callback',
  );

  while ($result = db_fetch_object($results)) {
    if (($result->link_num == 0 && $form_state['clicked_button']['#post']['only-new']) || (!$form_state['clicked_button']['#post']['only-new'])) {
      $batch['operations'][] = array('_inbound_checker_get_links_with_batch', array($result));
    }
  }
  batch_set($batch);

}

