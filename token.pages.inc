<?php

/**
 * @file
 * User page callbacks for the token module.
 */
use Drupal\Component\Serialization\Json;
use Drupal\Component\Utility\DiffArray;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\Core\Utility\Token;

/**
 * Theme a link to a token tree shown as a dialog.
 */
function template_preprocess_token_tree_link(&$variables) {
  if (empty($variables['text'])) {
    $variables['text'] = t('Browse available tokens.');
  }

  $variables['#attached']['library'][] = 'core/drupal.dialog.ajax';
  $variables['#attached']['library'][] = 'token/token';
  $variables['options']['attributes']['class'][] = 'token-dialog';
  $variables['options']['attributes']['class'][] = 'use-ajax';

  $info = token_theme();
  $tree_variables = array_intersect_key($variables, $info['token_tree']['variables']);
  $tree_variables = DiffArray::diffAssocRecursive($tree_variables, $info['token_tree']['variables']);
  if (!isset($variables['options']['query']['options'])) {
    $variables['options']['query']['options'] = array();
  }
  $variables['options']['query']['options'] += $tree_variables;

  // Because PHP converts query strings with arrays into a different syntax on
  // the next request, the options have to be encoded with JSON in the query
  // string so that we can reliably decode it for token comparison.
  $variables['options']['query']['options'] = Json::encode($variables['options']['query']['options']);

  // Set the token tree to open in a separate window.
  $variables['options']['attributes'] += [
    'data-dialog-type' => 'dialog',
    'data-dialog-options' => json_encode([
      'width' => 600,
      'height' => 400,
      'position' => ['my' => 'right bottom', 'at' => 'right bottom'],
      'draggable' => TRUE,
      'autoResize' => FALSE,
    ]),
   ];

  $variables['link'] = Link::createFromRoute($variables['text'], 'token.tree', [], $variables['options'])->toRenderable();
  $variables['url'] = new Url('token.tree', [], $variables['options']);
  $variables['attributes'] = $variables['options']['attributes'];
}

/**
 * Provide a 'tree' display of nested tokens.
 *
 * @ingroup themeable
 */
function theme_token_tree($variables) {
  /** @var \Drupal\token\Token $token_service */
  $token_service = \Drupal::token();
  $token_types = $variables['token_types'];
  $info = $token_service->getInfo();

  if ($token_types == 'all') {
    $token_types = array_keys($info['types']);
  }
  elseif ($variables['global_types']) {
    $token_types = array_merge($token_types, $token_service->getGlobalTokenTypes());
  }

  $element = array(
    /*'#cache' => array(
      'cid' => 'tree-rendered:' . hash('sha256', serialize(array('token_types' => $token_types, 'global_types' => NULL) + $variables)),
      'tags' => array(Token::TOKEN_INFO_CACHE_TAG),
    ),*/
  );

  // @todo Find a way to use the render cache for this.
  /*if ($cached_output = token_render_cache_get($element)) {
    return $cached_output;
  }*/

  $options = array(
    'flat' => TRUE,
    'restricted' => $variables['show_restricted'],
    'depth' => $variables['recursion_limit'],
  );

  $token_tree = [];

  foreach ($info['types'] as $type => $type_info) {
    if (!in_array($type, $token_types)) {
      continue;
    }

    $token_tree[$type] = $type_info;
    $token_tree[$type]['tokens'] = [];

    $tree = \Drupal::service('token.tree_builder')->buildTree($type, $options);
    foreach ($tree as $token => $token_info) {
      $token_tree[$type]['tokens'][$token] = $token_info;
    }
  }

  $element += array(
    '#type' => 'token_tree_table',
    '#token_tree' => $token_tree,
    '#show_restricted' => $variables['show_restricted'],
    '#columns' => ['name', 'token', 'description'],
    '#empty' => t('No tokens available'),
  );

  if ($variables['click_insert']) {
    $element['#caption'] = t('Click a token to insert it into the field you\'ve last clicked.');
    $element['#attributes']['class'][] = 'token-click-insert';
  }

  $output = drupal_render($element);
  //token_render_cache_set($output, $element);
  return $output;
}
