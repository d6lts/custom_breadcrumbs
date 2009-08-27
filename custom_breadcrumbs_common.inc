<?php
// $Id$

/**
 * @file
 * Common helper functions used by custom breadcrumbs submodules.
 */

/**
 * Helper function to determine if two paths match, allowing for wildcards and aliases.
 * @param $curpath and $breadcrumb_path are the two paths to compare
 * @return
 *    TRUE (1) if the paths match, FALSE (0) otherwise.
 */
function _custom_breadcrumbs_match_path($curpath, $breadcrumb_path) {
  $path = drupal_get_path_alias($curpath);
  // Compare with the internal and path alias (if any).
  $page_match = drupal_match_path(strtolower($path), strtolower($breadcrumb_path));
  if ($path != $curpath) {
   $page_match = $page_match || drupal_match_path(strtolower($curpath), strtolower($breadcrumb_path));
  }
  return $page_match;
}

/**
 * Helper function to construct the view path replacing wildcards with arguments.
 * @param $display
 *   The view $dipslay variable.
 * @param $viewsargs
 *   The $view->args variable.
 * @return $viewpath
 *   The complete path to the view.
 */
function _custom_breadcrumbs_construct_view_path($display, $viewargs) {
  $bits = explode('/', $display->display_options['path']);
  foreach ($bits as $pos => $bit) {
    if ($bit == '%' && !empty($viewargs)) {
      $bits[$pos] = array_shift($viewargs);
    }
  }
  if (!empty($viewargs)) {
    // Add any additional arguments to end of path.
    $bits = array_merge($bits, $viewargs);
  }
  $viewpath = implode('/', $bits);
  return $viewpath;
}

/**
 *  Helper function to specify allowed views displays for breadcrumb placement.
 * @param $display
 *   The view $dipslay variable.
 * @return
 *    TRUE if the display should be considered for a custom breadcrumb, FALSE otherwise.
 */
function _custom_breadcrumbs_allowed_display($display) {

  if (drupal_substr($display->id, 0, 4) == 'page' || drupal_substr($display->id, 0, 8) == 'calendar') {
    if (!(isset($display->handler->view->is_attachment) && $display->handler->view->is_attachment)) {
      if (isset($display->display_options['path']) ) {
        if (module_exists('panels') && panels_get_current_page_display()) {
          return FALSE;
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
 *  Helper function to obtain the appropriate type and value of each view argument.
 * @param $display_argument_ids
 *   An array of ids for each argument of the view.
 * @param $viewargs
 *   The $display->handler->view->args variable.
 * @return $arg_values
 *   An associate array of two elements, 'types' and 'values', each an array with elements corresponding to the views arguments.
 */
function _custom_breadcrumbs_views_parse_args($display_argument_ids, $viewargs) {
  $arg_values = array('types' => array(), 'values' => array());
  foreach ($display_argument_ids as $arg_id) {
    if (!empty($viewargs)) {
      $arg = array_pop($viewargs);
      switch ($arg_id) {
        case 'tid':
        case 'term_node_tid_depth':
          $terms = taxonomy_terms_parse_string($arg);
          $arg_values['types'][] = 'tid';
          $arg_values['values'][] = empty($terms['tids']) ? NULL : $terms['tids'][0];
          break;
        case 'name':
          if (drupal_substr($argument['table'], 0, 5) == 'term_') {
            $terms = taxonomy_get_term_by_name($arg);
            $arg_values['types'][] = 'tid';
            $arg_values['values'][] = empty($terms) ? NULL : $terms[0]->tid;
          }
          break;
        case 'vid':
        case 'uid':
        case 'nid':
          $arg_values['types'][] = $arg_id;
          $arg_values['values'][] = $arg;
          break;
      }
    }
  }
  return $arg_values;
}

/**
 *  Helper function to obtain the appropriate objects for token type replacement for this view.
 * @param $display
 *   The view $dipslay variable.
 * @return $objs
 *   An associate array of objects to use for token replacement.
 */
function _custom_breadcrumbs_views_token_types($display) {
  $objs = array();
  $display_argument_ids = array();
  // Check to see if the current display has overriden the default arguments.
  if (!empty($display->handler->view->display[$display->id]->display_options['arguments'])) {
    $display_argument_ids = array_keys($display->handler->view->display[$display->id]->display_options['arguments']);
  } // Otherwise, check the default display.
  elseif (!empty($display->handler->view->display['default']->display_options['arguments'])) {
    $display_argument_ids = array_keys($display->handler->view->display['default']->display_options['arguments']);
  }
  if (isset($display_argument_ids)) {
    $viewargs = (isset($display->handler->view->args) && is_array($display->handler->view->args)) ? $display->handler->view->args : array();
    $arg_values = _custom_breadcrumbs_views_parse_args($display_argument_ids, $viewargs);
    foreach ($arg_values['types'] as $key => $type) {
      switch ($type) {
        case 'tid':
          $objs['taxonomy'] = taxonomy_get_term($arg_values['values'][$key]);
          break;
        case 'nid':
          $objs['node'] = node_load(array('nid' => $arg_values['values'][$key]));
          break;
        case 'uid':
          $objs['user'] = user_load(array('uid' => $arg_values['values'][$key]));
          break;
      }
    }
  }
  return $objs;
}
