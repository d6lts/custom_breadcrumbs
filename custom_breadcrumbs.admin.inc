<?php
// $Id$

/**
 * @file
 * Admin page callback file for the custom_breadcrumbs module.
 */

/**
 *  Lists all current custom breadcrumbs, and provides a link to the edit page.
 */
function custom_breadcrumbs_page() {
  $custom_breadcrumbs_sort = variable_get('custom_breadcrumbs_sort', array(
    'direction' => array('name' => 'asc', 'breadcrumb_type' => 'asc', 'language' => 'asc'),
    'column' => 'name'));
  $sort = $custom_breadcrumbs_sort['direction']; 
  $multilingual = _custom_breadcrumbs_multilingual();
  // read session variable to load breadcrumbs of type and language selected by user
  $breadcrumbs = array();
  if (empty($_SESSION['custom_breadcrumbs_overview_filter'])) {
    $breadcrumbs = _custom_breadcrumbs_load_all_breadcrumbs(TRUE);
  } 
  else {
    $modules = array();
    $languages = array();
    foreach ($_SESSION['custom_breadcrumbs_overview_filter'] as $filter) {
      if ($filter[0] == 'type') {
        $modules[] = $filter[1];
      }
      elseif ($filter[0] == 'language') {
        $languages[] = $filter[1];
      }
    }
    if (empty($modules)) {
      $modules =  module_implements('cb_breadcrumb_info');   
    }
    foreach ($modules as $module) {
      $more = custom_breadcrumbs_load_breadcrumbs_bid($module, NULL, $languages);
      if (!empty($more)) {
        $breadcrumbs = array_merge($more, $breadcrumbs);
      }
    }
  }

  // Sort the breadcrumbs according to $sort and $order.

  $order = in_array($_GET['order'], array('name', 'breadcrumb_type', 'language')) ? $_GET['order'] : 'name';
  $sort[$order] = (isset($_GET['sort']) && ($_GET['sort'] == 'desc')) ? 'desc' : 'asc';

  variable_set('custom_breadcrumbs_sort', array('direction' => $sort, 'column' => $order));
  usort($breadcrumbs, '_custom_breadcrumbs_sort_cmp');

  $rows = array();
  foreach ($breadcrumbs as $breadcrumb) {
    $row = array();
    $name = $breadcrumb->name;
    $type = $breadcrumb->breadcrumb_type;
    $row[] = $name . (!empty($breadcrumb->visibility_php) ? ' '. t('with PHP snippet') : '');
    $row[] = $type;
    if ($multilingual) {
      $row[] = module_invoke('locale', 'language_name', $breadcrumb->language);
    }
    $row[] =  l(t('edit'), 'admin/build/custom_breadcrumbs/'. $breadcrumb->breadcrumb_type .'/edit/'. $breadcrumb->bid);
    $rows[] = $row;
  }
  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('No custom breadcrumbs have been defined.'), 'colspan' => 2 + (int)$multilingual));
  }

  // Make the breadcrumb list table sortable by name, type and language.

  $path = 'admin/build/custom_breadcrumbs';
  $sort[$order] = ($sort[$order] == 'asc') ? 'desc' : 'asc';
  $header = array(); 
  $header[] = l('Name', $path, array('attributes' => array('title' => t('sort by Name')), 'query' => 'order=name&sort='. $sort['name']));
  $header[] = l('Page type', $path, array('attributes' => array('title' => t('sort by Page Type')), 'query' => 'order=breadcrumb_type&sort='. $sort['breadcrumb_type']));
  if ($multilingual) {
    $header[] = l('Language', $path,  array('attributes' => array('title' => t('sort by Language')), 'query' => 'order=language&sort='. $sort['language']));
  }
  $header[] = t('Operations');
  $output =  drupal_get_form('custom_breadcrumbs_filter_form');
  $output .= theme('table', $header, $rows);

  return $output;
}

/**
 * Helper function for usort to sort two breadcrumb objects by name, type, or language.
 * Each column can be independently sorted as asc or desc.
 */
function _custom_breadcrumbs_sort_cmp($bc1, $bc2) {
  $custom_breadcrumbs_sort = variable_get('custom_breadcrumbs_sort', array(
    'direction' => array('name' => 'asc', 'breadcrumb_type' => 'asc', 'language' => 'asc'),
    'column' => 'name')
  );
  $options = array('name', 'breadcrumb_type', 'language'); 
  $first = $custom_breadcrumbs_sort['column'];
  $key = array_pop(array_keys($options, $first));
  unset($options[$key]);
  // Reindex the array.
  $options = array_values($options);
  $sortdir = array();
  foreach ($custom_breadcrumbs_sort['direction'] as $key => $sort) {
    $sortdir[$key] = ($sort == 'asc') ? 1 : -1;
  }
  if ($bc1->$first == $bc2->$first) {
    if ($bc1->$options[0] == $bc2->$options[0]) {
      if ($bc1->$options[1] == $bc2->$options[1]) {
        return 0;
      }  
      return ($bc1->$options[1] > $bc2->$options[1]) ? $sortdir[$options[1]] : -1*$sortdir[$options[1]];
    }
    return ($bc1->$options[0] > $bc2->$options[0]) ? $sortdir[$options[0]] : -1*$sortdir[$options[0]];
  }
  return ($bc1->$first > $bc2->$first) ? $sortdir[$first] : -1*$sortdir[$first];
}

/**
 * Display an edit form for a custom breadcrumb record.
 */
function custom_breadcrumbs_form(&$form_state, $type) {
  $form = array();
  $breadcrumb = NULL;
  $bid = arg(5);
  if (isset($bid)) {
    drupal_set_title('Edit Custom Breadcrumb for Node');
    $breadcrumb = array_pop(custom_breadcrumbs_load_breadcrumbs_bid('custom_breadcrumbs', $bid));
  }
  else {
    drupal_set_title('Add Custom Breadcrumb for Node');
  }
  $options = array();
  foreach (node_get_types('names') as $type => $name) {
    $options[$type] = $name;
  }
  $form['node_type'] = array(
    '#type' => 'select',
    '#title' => t('Node type'),
    '#required' => TRUE,
    '#options' => $options,
    '#description' => t('The node type this custom breadcrumb trail will apply to.'),
    '#default_value' => $bid ? $breadcrumb->node_type : NULL,
    '#weight' => -10,
  );
  // Store information needed to save this breadcrumb.
  $form['#table'] = 'custom_breadcrumb';
  $form += custom_breadcrumbs_common_form_elements($bid, $breadcrumb);

  return $form;
}

/**
 * Form builder; Configure basic and advanced custom breadcrumbs settings for this site.
 *
 * @ingroup forms
 * @see system_settings_form()
 */

function custom_breadcrumbs_admin_settings() {
  $form = array();
  drupal_set_title(t('Custom Breadcrumbs Configuration'));

  $form['settings'] = array(
    '#type'           => 'fieldset',
    '#title'          => t('Basic Settings'),
    '#collapsible'    => TRUE,
  );

  $form['settings']['custom_breadcrumb_home'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Home breadcrumb text'),
    '#default_value'  => variable_get('custom_breadcrumb_home', t('Home')),
    '#description'    => t('Text to display at top of breadcrumb trail.  Typically home or your site name.  Leave blank to have no home breadcrumb.'),
  );

  $form['adv_settings'] = array(
    '#type'           => 'fieldset',
    '#title'          => t('Advanced Settings'),
    '#collapsible'    => TRUE,
  );

  $form['adv_settings']['custom_breadcrumbs_force_active_trail'] = array(
    '#type' => 'checkbox',
    '#title' => t('Force the active trail'),
    '#description' => t('This options sets the active trail to match the custom breadcrumb trail and overrides the normal theme_links() implementation to add the active-trail class to links. This is experimental and may not work as expected. If this is of interest to you, please test and report back to the custom breadcrumbs issue queue.'),
    '#default_value' => variable_get('custom_breadcrumbs_force_active_trail', FALSE),
    '#weight' => -10,
  );

  $form['#submit'][] = 'custom_breadcrumbs_admin_settings_submit';

  return system_settings_form($form);
}

function custom_breadcrumbs_admin_settings_submit($form, &$form_state) {
  // Rebuild the theme registry.
  drupal_rebuild_theme_registry();
  drupal_set_message('The theme registry has been rebuilt');
}

/*
 * Provides form elements commonly used by custom breadcrumbs submodules.
 */
function custom_breadcrumbs_common_form_elements($bid, $breadcrumb) {
  $form = array();
  $form['bid'] = array('#type' => 'hidden', '#value' => $bid, );
  $multilingual = _custom_breadcrumbs_multilingual();
  if ($multilingual) {
    $form['language'] = array(
      '#type' => 'select',
      '#title' => t('Language'),
      '#options' => array('' => t('All languages')) + locale_language_list('name'),
      '#default_value' => $bid ? $breadcrumb->language : NULL,
      '#description' => t('A breadcrumb set for a specific language will always be used when displaying a node in that language, and takes precedence over breadcrumbs set for <em>All languages</em>.'),
    );
  }
  else {
    $form['language'] = array('#type' => 'value', '#value' => '', );
  }
  $form['visibility_php'] = array(
    '#type' => 'textarea',
    '#title' => t('Breadcrumb visibility'),
    '#access' => user_access('use php in custom breadcrumbs'),
    '#description' => t('Determine whether this breadcrumb should be displayed by using a snippet of PHP to return TRUE or FALSE. Note that this code has access to the $node variable, and can check its type or any other property.'),
    '#default_value' => $bid ? $breadcrumb->visibility_php : '',
  );
  $form['titles'] = array(
    '#type' => 'textarea',
    '#title' => t('Titles'),
    '#required' => TRUE,
    '#description' => t('A list of titles for the breadcrumb links, one on each line.'),
    '#default_value' => $bid ? $breadcrumb->titles : NULL,
  );
  $form['paths'] = array(
    '#type' => 'textarea',
    '#title' => t('Paths'),
    '#required' => TRUE,
    '#description' => t('A list of Drupal paths for the breadcrumb links, one on each line.'),
    '#default_value' => $bid ? $breadcrumb->paths : NULL,
  );
  $form['help'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Placeholder tokens'),
    '#description' => t("The following placeholder tokens can be used in both paths and titles. When used in a path or title, they will be replaced with the appropriate values."),
  );
  if (module_exists('token')) {
    // This code depends on a patch still under review at http://drupal.org/node/127072.
    // $form['help']['tokens'] = array('#value' => theme('token_help', array('global', 'node', 'user')), );
    // This is overkill, but is necessary until this patch is committed.
    $form['help']['tokens'] = array('#value' => theme('token_help', 'all'), );
  }
  else {
    $form['help']['#description'] = t("To use dynamic placeholder tokens in your breadcrumb trails (the ID or title of the current node, for example), download and install the <a href='@token'>Token module</a> from Drupal.org.", array('@token' => 'http://www.drupal.org/project/token'));
    $form['help']['#collapsible'] = FALSE;
    $form['help']['#collapsed'] = FALSE;
  }
  $form['help2'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Special identifiers'),
    '#description' => t("The following special identifiers can be used to achieve a special behavior. Identifiers should be added to the paths area in the following format:  identifier|path. <br />Example: ". check_plain('<pathauto>') ."|[ogname-raw]"),
  );
  $form['help2']['tokens'] = array('#value' => theme('custom_breadcrumbs_help_identifiers'), );

  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  if ($bid) {
    $form['buttons']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('custom_breadcrumbs_form_delete'),
    );
  }
  $form['buttons']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('custom_breadcrumbs_form_cancel'),
  );
  return $form;
}

function custom_breadcrumbs_form_validate($form, &$form_state) {
  $path_count = count(explode("\n", trim($form_state['values']['paths'])));
  $title_count = count(explode("\n", trim($form_state['values']['titles'])));
  if ($title_count != $path_count) {
    $error_field = ($title_count < $path_count) ? 'titles' : 'paths';
    form_set_error($error_field, t('Every link path must have a matching title. There are !paths paths, and !titles titles.', array('!paths' => $path_count, '!titles' => $title_count)));
  }
}

function custom_breadcrumbs_form_submit($form, &$form_state) {
  $breadcrumb = (object)$form_state['values'];
  _custom_breadcrumbs_save_breadcrumb($form['#table'], $breadcrumb, $form['#save_func']);
  $form_state['redirect'] = 'admin/build/custom_breadcrumbs';
}

function custom_breadcrumbs_form_delete($form, &$form_state) {
  _custom_breadcrumbs_delete_breadcrumb($form['#table'], $form_state['values']['bid'], $form['#delete_func']);
  $form_state['redirect'] = 'admin/build/custom_breadcrumbs';
}

function custom_breadcrumbs_form_cancel($form, &$form_state) {
  $form_state['redirect'] = 'admin/build/custom_breadcrumbs';
}

/**
 * Check whether the administration interface should show multilingual features.
 */
function _custom_breadcrumbs_multilingual() {
  return module_exists('locale'); // || db_result(db_query("SELECT COUNT(*) FROM {custom_breadcrumb} WHERE language != ''"));
}

/**
 * List custom_breadcrumbs administration filters that can be applied.
 */
function custom_breadcrumbs_filters() {
  $filters = array();
  $options = array();
  foreach (module_implements('cb_breadcrumb_info') as $module) {
    $info = module_invoke($module, 'cb_breadcrumb_info');  
    $options[$module] = $info['type'];
  }
  $filters['type'] = array(
    'title' => t('Page type'),
    'options' => $options, 
  );
  if (_custom_breadcrumbs_multilingual()) {
    $filters['language'] = array(
      'title' => t('Language'),
      'options' => array('' => t('All languages')) + locale_language_list('name'),
    );
  }
  return $filters;
}

/**
 * Form builder; Return form for custom_breadcrumbs administration filters.
 *
 * @ingroup forms
 * @see custom_breadcrumbs_filter_form_submit()
 */
function custom_breadcrumbs_filter_form() {
  $session = &$_SESSION['custom_breadcrumbs_overview_filter'];
  $session = is_array($session) ? $session : array();
  $filters = custom_breadcrumbs_filters();

  $i = 0;
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only custom_breadcrumbs where'),
    '#theme' => 'custom_breadcrumbs_filters',
  );
  foreach ($session as $filter) {
    list($type, $value) = $filter;
    // Merge an array of arrays into one if necessary.
    $options = $filters[$type]['options'];
    $params = array('%property' => $filters[$type]['title'] , '%value' => $options[$value]);
    if ($i++ > 0) {
      $form['filters']['current'][] = array('#value' => t('<em>and</em> where <strong>%property</strong> is <strong>%value</strong>', $params));
    }
    else {
      $form['filters']['current'][] = array('#value' => t('<strong>%property</strong> is <strong>%value</strong>', $params));
    }
    unset($filters[$type]);
  }

  foreach ($filters as $key => $filter) {
    $names[$key] = $filter['title'];
    $form['filters']['status'][$key] = array(
      '#type' => 'select',
      '#options' => $filter['options'],
    );
  }

  $form['filters']['filter'] = array(
    '#type' => 'radios',
    '#options' => $names,
  );

  if (count($filters)) {
    $form['filters']['buttons']['submit'] = array(
      '#type' => 'submit',
      '#value' => (count($session) ? t('Refine') : t('Filter')),
    );
  }
  if (count($session)) {
    $form['filters']['buttons']['undo'] = array(
      '#type' => 'submit',
      '#value' => t('Undo'),
    );
    $form['filters']['buttons']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }

  drupal_add_js('misc/form.js', 'core');

  return $form;
}

/**
 * Process result from custom_breadcrumbs administration filter form.
 */
function custom_breadcrumbs_filter_form_submit($form, &$form_state) {
  $op = $form_state['values']['op'];
  $filters = custom_breadcrumbs_filters();
  switch ($op) {
    case t('Filter'): case t('Refine'):
      if (isset($form_state['values']['filter'])) {
        $filter = $form_state['values']['filter'];
        // Merge an array of arrays into one if necessary.
        $options = $filters[$filter]['options'];
        if (isset($options[$form_state['values'][$filter]])) {
          $_SESSION['custom_breadcrumbs_overview_filter'][] = array($filter, $form_state['values'][$filter]);
        }
      }
      break;
    case t('Undo'):
      array_pop($_SESSION['custom_breadcrumbs_overview_filter']);
      break;
    case t('Reset'):
      $_SESSION['custom_breadcrumbs_overview_filter'] = array();
      break;
    case t('Update'):
      return;
  }

  $form_state['redirect'] = 'admin/build/custom_breadcrumbs';
  return;
}

/**
 * Theme custom_breadcrumbs filter form.
 *
 * @ingroup themeable
 */
function theme_custom_breadcrumbs_filter_form($form) {
  $output = '<div id="custom-breadcrumbs-filter">';
  $output .= drupal_render($form['filters']);
  $output .= '</div>';
  $output .= drupal_render($form);
  return $output;
}

/**
 * Theme custom_breadcrumbs filter selector.
 *
 * @ingroup themeable
 */
function theme_custom_breadcrumbs_filters($form) {
  $output = '<ul class="clear-block">';
  if (!empty($form['current'])) {
    foreach (element_children($form['current']) as $key) {
      $output .= '<li>'. drupal_render($form['current'][$key]) .'</li>';
    }
  }
  $output .= '<li>';
  if (isset($form['buttons']['submit'])) {
    $output .= '<dl class="multiselect">'. (!empty($form['current']) ? '<dt><em>'. t('and') .'</em> '. t('where') .'</dt>' : '') .'<dd class="a">';
    foreach (element_children($form['filter']) as $key) {
      $output .= drupal_render($form['filter'][$key]);
    }
    $output .= '</dd>';
    $output .= '<dt>'. t('is') .'</dt><dd class="b">';
    foreach (element_children($form['status']) as $key) {
      $output .= drupal_render($form['status'][$key]);
    }
    $output .= '</dd>';
    $output .= '</dl>';
  }  
  else {
    $output .= '<li>';
  }
  $output .= '<div class="container-inline" id="custom-breadcrumbs-admin-buttons">'. drupal_render($form['buttons']) .'</div>';
  $output .= '</li></ul>';

  return $output;
}
