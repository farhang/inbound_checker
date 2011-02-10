<?php
// $Id$

/**
 * @file
 * Contains Drupal form API callbacks & helpers.
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

  $form['inbound_checker_site_expiry'] = array(
    '#type' => 'date',
    '#title' => t('Expiry date'),
    '#description' => t('Specify the date in which your subscription (if any) will be expired.'),
    // Alter the year range.
    '#after_build' => array('_inbound_checker_date_after_build'),
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
 *
 * @see _inbound_checker_site_save()
 */
function inbound_checker_form_add_submit($form, &$form_state) {
  $link = new stdClass();
  $link->site_name = $form_state['values']['inbound_checker_site_name'];
  $link->site_address = $form_state['values']['inbound_checker_site_address'];
  $link->expiry = _inbound_checker_mktime($form_state['values']['inbound_checker_site_expiry']);

  // Saves the inbound link.
  _inbound_checker_site_save($link);

  // Notify.
  drupal_set_message(t('Website entry has been successfully created.'));
}

/**
 * Form callback sites list page.
 */
function inbound_checker_form_list() {
  $form = array();

  // Inject themed table of available sites.
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
 * Validation callback for sites list form.
 */
function inbound_checker_form_list_validate($form, &$form_state) {

  // Check the keyword text field in settings tab for not empty.
if (!variable_get('inbound_checker_keyword', '')) {
    form_set_error('inbound_checker_check_zeros', t('Please fill the keyword textfield in settings tab.'));
  }
}

/**
 * Submission callback for sites list form.
 */
function inbound_checker_form_list_submit($form, &$form_state) {
  // TODO: Check the logic. Assigned to @sepehr.

  $batch = array(
    'operations' => array(),
    'title' => t('Inbound Links Checking...'),
    'init_message' => t('Initializing inbound link checking...'),
    'error_message' => t('An error occured while checking inbound links.'),
    'progress_message' => t('@current out of @total links has been checked, @remaining remaining. (@percentage%)'),
  );

  // TODO: Use module's API.
  $results = db_query("SELECT * FROM {inbound_checker}");

  while ($result = db_fetch_object($results)) {
    // TODO: Check $form_state['values'] instead. **Was done**
    if (($result->icid == 0 && $form_state['values']['inbound_checker_check_zeros']) || (!$form_state['values']['inbound_checker_check_zeros'])) {
      $batch['operations'][] = array('_inbound_checker_getlinks_batch', array($result)); //TODO: Rename batch helper. **Was done**
    }
  }

  if($batch['operations'] == NULL) {
    form_set_error ('inbound_checker_check_zeros' ,t('No links to check.'));
  }
  else {
    batch_set($batch);
  }
}

/**
 * Date field after_build callback to workaround the date field stupid limitation.
 *
 * @param $form_element
 *   Date field element.
 * @param $form_state
 *   State of the form array.
 *
 * @return
 *   Updated $form_element.
 *
 * @see expand_date()
 */
function _inbound_checker_date_after_build($form_element, &$form_state) {
  $now = format_date(time(), 'custom', 'Y');
  $form_element['year']['#options'] = drupal_map_assoc(range($now, $now + 5));
  return $form_element;
}

/**
 * Helper function aasdasd a timestamp from date and time form elements.
 *
 * @param $date_element
 *   An array of form date element:
 *   - year
 *   - month
 *   - day
 *
 * @param $time_element
 *   24-format hour integer.
 *
 * @return
 *   Corresponding timestamp.
 */
function _inbound_checker_mktime($date_element, $time_element = 0) {
  return mktime(
    (int) $time_element, 0, 0,
    (int) $date_element['month'],
    (int) $date_element['day'],
    (int) $date_element['year']
  );
}

