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
 * Form callback for site entry add/edit form.
 *
 * @param $form_state
 *   Known form state array as provided bu drupal_get_form().
 * @param $ic
 *   Inbound checker object. If provided it will be the edit form.
 *
 * @return
 *   Defined form array.
 *
 * @ingroup forms
 */
function inbound_checker_form($form_state, $ic = NULL) {
  $form = array();
  // Site entry name.
  $form['inbound_checker_site_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Website name'),
    '#description' => t('Name of the external website you wish to check.'),
    '#default_value' => isset($ic->site_name) ? $ic->site_name : '',
    '#required' => TRUE,
  );
  // Site entry URL.
  $form['inbound_checker_site_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Website address'),
    '#description' => t('The external site URL to seek the keyword in. e.g.:‌ http://www.example.com'),
    '#default_value' => isset($ic->site_address) ? $ic->site_address : '',
    '#required' => TRUE,
  );
  // Site entry link expiry date.
  // TODO: @farhang: Consider links that get not expired, ever!
  $form['inbound_checker_site_expiry'] = array(
    '#type' => 'date',
    '#title' => t('Expiry date'),
    '#description' => t('Specify the date in which your subscription (if any) will be expired.'),
    // Alter the year range.
    '#after_build' => array('_inbound_checker_date_after_build'),
    '#default_value' => !isset($ic->expiry) ? '' : array(
      'day'   => date('j', $ic->expiry),
      'year'  => date('Y', $ic->expiry),
      'month' => date('m', $ic->expiry),
    ),
  );
  // Site entry inbound link price.
  $form['inbound_checker_link_price'] = array(
    '#type' => 'textfield',
    '#title' => t('Link price'),
    '#description' => t('Optional inbound link price you have payed to get.'),
    '#default_value' => isset($ic->price) ? $ic->price : '0',
  );
  // Site entry identifier in case that it's a edit form.
  $form['inbound_checker_id'] = array(
    '#type' => 'hidden',
    '#default_value' => isset($ic->icid) ? $ic->icid : '',
  );
  // Submit button.
  $form['op'] = array(
    '#type' => 'submit',
    '#name' => isset($ic) ? 'edit' : 'add',
    '#value' => isset($ic) ? t('Edit') : t('Add'),
  );

  return $form;
}

/**
 * Validation callback for site add/edit form.
 */
function inbound_checker_form_validate($form, &$form_state) {
  // TODO: @sepehr: Check the logic.
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

  // TODO: @sepehr: Define the module API functions and do such operations through that layer.
  if($form_state['values']['op'] == t('Add')) {
    $results = db_query("SELECT * FROM {inbound_checker} WHERE site_address = '%s'", $form_state['values']['inbound_checker_site_address']);
    $result = db_fetch_object($results);
    if ($result) {
      form_set_error('inbound_checker_site_address', t('Your url already existes.'));
    }
  }
}

/**
 * Submission callback for site add/edit form.
 *
 * @see _inbound_checker_site_save()
 */
function inbound_checker_form_submit($form, &$form_state) {
  // FIX: @farhang: Aftering editing an entry, I got redirected to "E" path.

  $link = new stdClass();
  $link->site_name = $form_state['values']['inbound_checker_site_name'];
  $link->site_address = $form_state['values']['inbound_checker_site_address'];
  $link->expiry = _inbound_checker_mktime($form_state['values']['inbound_checker_site_expiry']);
  $link->price = $form_state['values']['inbound_checker_link_price'];
  if($form_state['values']['inbound_checker_id'] != '') {
    $link->icid = $form_state['values']['inbound_checker_id'];
  }

  // Saves the inbound link.
  _inbound_checker_site_save($link);

  // Notify.
  drupal_set_message(t('Website entry has been successfully created.'));

  // FIX: @farhang: That's not the way to redirect user in this case.
  drupal_goto('admin/reports/inbound-links');
}

/**
 * Form callback for site entries listing.
 *
 * @ingroup forms
 */
function inbound_checker_form_list() {
  $form = array();

  // Inject themed table of available sites.
  $form['#prefix'] = _inbound_checker_sites_table();

  // TODO: @sepehr: Use API layer instead.
  $count = db_result(db_query("SELECT COUNT(icid) FROM {inbound_checker}"));
  if ($count) {
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
  }

  return $form;
}

/**
 * Validation callback for sites list form.
 */
function inbound_checker_form_list_validate($form, &$form_state) {
  // Check the keyword text field in settings tab for it isn't empty.
  if (!variable_get('inbound_checker_keyword', '')) {
    form_set_error('inbound_checker_check_zeros', t('Please fill the keyword textfield in settings tab.'));
  }
  // Check available sites.
  // TODO: @sepehr: Use module's API when ready.
  if ($form_state['values']['inbound_checker_check_zeros']) {
    $results = db_result(db_query("SELECT COUNT(icid) FROM {inbound_checker} WHERE count = 0"));
    if (!$results) {
      form_set_error('inbound_checker_check_zeros', t('There were no links to check.'));
    }
  }
}

/**
 * Submission callback for sites list form.
 */
function inbound_checker_form_list_submit($form, &$form_state) {
  // TODO: @sepehr: Check the logic.
  // FIX: @farhang: When zero links checkbox is checked, nothing will happen through the batch process.
  // FIX: @farhang: Google/Alexa pageranks calculation failure.
  $batch = array(
    'operations' => array(),
    'title' => t('Inbound Links Checking...'),
    'init_message' => t('Initializing inbound link checking...'),
    'error_message' => t('An error occured while checking inbound links.'),
    'progress_message' => t('@current out of @total links has been checked, @remaining remaining. (@percentage%)'),
  );

  // TODO: @sepehr: Use module's API.
  $results = db_query("SELECT * FROM {inbound_checker}");

  while ($result = db_fetch_object($results)) {
    if (($result->icid == 0 && $form_state['values']['inbound_checker_check_zeros']) || (!$form_state['values']['inbound_checker_check_zeros'])) {
      $batch['operations'][] = array('_inbound_checker_getlinks_batch', array($result));
    }
  }

  batch_set($batch);
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
  // TODO: @farhang: Read.
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

/**
 * Helper function to theme a table for sites listing.
 *
 * @return
 *   Sites themed output.
 */
function _inbound_checker_sites_table() {
  // TODO: @sepehr: Check the logic.
  // TODO: @sepehr: Consider implementing hook_theme(), if required.
  $rows = array();
  $header = array(
    '',
    array(
      'data' => t('Site name'),
      'field' => 'site_name',
    ),
    t('Site address'),
    array(
      'data' => t('Number of links'),
      'field' => 'count',
    ),
    array(
      'data' => t('Google PR'),
      'field' => 'google_pr',
    ),
    array(
      'data' => t('Alexa PR'),
      'field' => 'alexa_pr',
    ),
    t('Domain age'),
    array(
      'data' => t('Link Price'),
      'field' => 'price',
    ),
    array(
      'data' => t('Expire date'),
      'field' => 'expiry',
    ),
    t('Actions'),
  );

  $results = db_query("SELECT * FROM {inbound_checker}");
  $link_class = '';
  $count = 1;
  $results = pager_query("SELECT * FROM {inbound_checker}" . tablesort_sql($header), 25, 1);
  while ($result = db_fetch_object($results)) {
    $class = ($result->icid > 0) ? 'ok' : 'warning';
    if ($result->expiry < time()) {
      $class = 'error';
    }
    $rows[] = array(
      'data' => array(
        $count,
        check_plain($result->site_name),
        check_plain($result->site_address),
        check_plain($result->count),
        check_plain($result->google_pr),
        check_plain($result->alexa_pr),
        check_plain($result->site_age),
        check_plain($result->price),
        check_plain(format_date($result->expiry, 'custom', 'D, m/d/Y')),
        $link = '<a href="inbound-links/' . $result->icid . '/edit"' . '>edit</a>',
      ),
      'class' => $class,
    );
    ++$count;
  }

  return empty($rows) ?
    t('Nothing to show, Add your website in "Add site" tab.') :
    theme('table', $header, $rows) . theme('pager', array(), 25, 1);
}

