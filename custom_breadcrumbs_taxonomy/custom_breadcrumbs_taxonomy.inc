<?php
// $Id$

/**
 * @file
 * Helper functions for custom_breadcrumbs_taxonomy.
 */

/**
 * Set a Taxonomy breadcrumb and call the original taxonomy/term/% callback.
 *
 * @param $callback
 *   A callback function as defined in hook_menu().
 * @param $file
 *   A callback file as defined in hook_menu().
 * @param $filepath
 *   A callback file path as defined in hook_menu().
 * @param $str_tids
 *   A term selector string, e.g. "1,3,8" or "4+9".
 * @param ...
 *   Additional arguments to pass on to the callback.
 *
 * @ return
 *   The return value of the original callback function.
 */
function custom_breadcrumbs_taxonomy_term_page() {
  $args = func_get_args();
  $callback = array_shift($args);
  $file = array_shift($args);
  $filepath = array_shift($args);
  $str_tids = array_shift($args);

  // Use first term to generate breadcrumb trail.
  $terms = taxonomy_terms_parse_string($str_tids);
  $breadcrumb = custom_breadcrumbs_taxonomy_generate_breadcrumb($terms['tids'][0], TRUE);

  require_once($filepath .'/'. $file);
  $output = call_user_func_array($callback, $args);

  if (custom_breadcrumbs_is_visible($breadcrumb)) {
    drupal_set_breadcrumb($breadcrumb);
  }

  return $output;

}

/**
 * Return lightest term for given node ($nid).
 * Similar to taxonomy_node_get_terms, but only return the lightest term in the
 * lightest vocab for the node.
 */
function custom_breadcrumbs_taxonomy_node_get_lightest_term($nid) {

  /* We only want the first row of the result--this is the lightest term of the
   * lightest vocab.
   */
  $result = db_query(db_rewrite_sql('SELECT t.* FROM {term_node} r INNER JOIN {term_data} t ON r.tid = t.tid INNER JOIN {term_hierarchy} h ON t.tid = h.tid INNER JOIN {vocabulary} v ON t.vid = v.vid WHERE r.nid = %d ORDER BY v.weight, h.parent DESC, t.weight, t.name', 't', 'tid'), $nid);

  // Extract the first row of query.
  $term = db_fetch_object($result);
  return $term;
}

/**
 * Generate a breadcrumb from the taxonomy hierarchy of the tid. This is a taxonomy_breadcrumbs type
 * breadcrumb, and will only be used on the taxonomy term page and on nodes if
 * custom_breadcrumbs_taxonomy_use_hierarchy has been enabled. 
 */
function custom_breadcrumbs_taxonomy_generate_breadcrumb($tid, $is_term_page = FALSE, $node = NULL) {
  $term = taxonomy_get_term($tid);

  // Prepare for token replacements.
  $types = array();
  $replace_tokens = module_exists('token');
  if ($replace_tokens) {
    // Do token replacement.
    global $user;
    if ($user->uid) {
      $user = user_load(array('uid' => $user->uid));
    }
    if ($node) {
      $types['node'] = $node;
    }
    $types['user'] = $user;
    $types['global'] = NULL;
    $types['taxonomy'] = $term;
  }

  // Generate the HOME breadcrumb.
  $home_text = variable_get('custom_breadcrumb_home', t('Home'));
  if ($home_text != '') {
    $trail = array(l($home_text, '<front>'));
  }
  // Generate the VOCABULARY breadcrumb.
  // Check to see if a vocabulary breadcrumb exists. 
  $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_vocabulary', array('vid' => $term->vid));
  $vocabulary_path = NULL;
  if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $node)) {
    $vocabulary_path = $breadcrumb->paths;
    if ($replace_tokens) {
      $vocabulary_path = token_replace_multiple($vocabulary_path, $types);
    }
  }
  if ($vocabulary_path != NULL) {
    $vocabulary = taxonomy_vocabulary_load($term->vid);
    $trail[] = l(_custom_breadcrumbs_taxonomy_tt("taxonomy:vocabulary:$term->tid:name", $vocabulary->name), $vocabulary_path);
  }
  // Generate the TERM breadcrumb.
  $parent_terms = array_reverse(taxonomy_get_parents_all($tid));
  foreach ($parent_terms as $parent_term) {
    $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_term', array('tid' => $parent_term->tid));
    $term_path = NULL;
    if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $node)) {
      $term_path = $breadcrumb->paths;
      if ($replace_tokens) {
        $term_path = token_replace_multiple($term_path, $types);
      }
    }
    if ($term_path == NULL) {
      $term_path = taxonomy_term_path(taxonomy_get_term($parent_term->tid));
    }
    // Do not create links to own self if we are on a taxonomy/term page.
    if ($is_term_page && $parent_term->tid == $tid) {
      $trail[] = _custom_breadcrumbs_taxonomy_tt("taxonomy:term:$parent_term->tid:name", $parent_term->name);
    }
    else {
      $trail[] = l(_custom_breadcrumbs_taxonomy_tt("taxonomy:term:$parent_term->tid:name", $parent_term->name), $term_path);
    }
  }
  // Optionally remove the current TERM from end of breadcrumb trail.
  if (!variable_get('custom_breadcrumbs_taxonomy_show_current_term', TRUE) && !is_null($breadcrumb)) {
      array_pop($trail);
  }
  return $trail;
}

/**
 * Helper function for when i18ntaxonomy module is not installed.
 */

function _custom_breadcrumbs_taxonomy_tt($string_id, $default, $language = NULL) {
  return function_exists('tt') ? tt($string_id, $default, $language) : $default;
}
