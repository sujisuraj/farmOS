<?php

/**
 * @file
 * Farm asset pages.
 */

/**
 * Asset view callback.
 *
 * @param FarmAsset $farm_asset
 *   The farm asset entity.
 *
 * @return array
 *   Returns the entity render array.
 */
function farm_asset_view(FarmAsset $farm_asset) {

  // Set the page title.
  drupal_set_title(entity_label('farm_asset', $farm_asset));

  // Build the asset's render array.
  $build = entity_view('farm_asset', array(entity_id('farm_asset', $farm_asset) => $farm_asset), 'full');

  // Return the render array.
  return $build;
}

/**
 * Page to select asset type to add new asset.
 */
function farm_asset_add_types_page() {
  $items = array();
  foreach (farm_asset_types() as $farm_asset_type_key => $farm_asset_type) {
    if (farm_asset_access('create', $farm_asset_type)) {
      $items[] = l(entity_label('farm_asset_type', $farm_asset_type), 'farm/asset/add/' . $farm_asset_type_key);
    }
  }
  return array(
    'list' => array(
      '#theme' => 'item_list',
      '#items' => $items,
      '#title' => t('Select a type of asset to create.'),
    ),
  );
}

/**
 * Add new asset page callback.
 *
 * @param string $type
 *   The farm asset type.
 *
 * @return array
 *   Returns a form array.
 */
function farm_asset_add($type) {
  $farm_asset_type = farm_asset_types($type);

  $farm_asset = entity_create('farm_asset', array('type' => $type));
  drupal_set_title(t('Add @name', array('@name' => entity_label('farm_asset_type', $farm_asset_type))));

  $output = drupal_get_form('farm_asset_form', $farm_asset);

  return $output;
}

/**
 * Asset form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 * @param FarmAsset $farm_asset
 *   The farm asset entity.
 *
 * @return array
 *   Returns a form array.
 */
function farm_asset_form(array $form, array &$form_state, FarmAsset $farm_asset) {

  $form['farm_asset'] = array(
    '#type' => 'value',
    '#value' => $farm_asset,
  );

  // Load the asset type.
  $farm_asset_type = farm_asset_type_load($farm_asset->type);

  // Asset name.
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Give this %type asset a name.', array('%type' => $farm_asset_type->label)),
    '#default_value' => $farm_asset->name,
    '#required' => TRUE,
    '#weight' => -100,
  );

  // Additional settings (vertical tabs at the bottom of the form).
  $form['additional_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 99,
  );

  // Asset archived status.
  $archived_boolean = FALSE;
  $archived_date = '';
  if (!empty($farm_asset->archived)) {
    $archived_boolean = TRUE;
    $archived_date = ' (' . format_date($farm_asset->archived) . ')';
  }
  $form['asset_status'] = array(
    '#type' => 'fieldset',
    '#title' => t('Asset status'),
    '#description' => t('Archive this asset. Archived assets will not show in most lists, but will be visible in archives.'),
    '#collapsible' => TRUE,
    '#group' => 'additional_settings',
  );
  $form['asset_status']['archived_boolean'] = array(
    '#type' => 'checkbox',
    '#title' => t('Archived') . $archived_date,
    '#default_value' => $archived_boolean,
  );
  $form['asset_status']['archived'] = array(
    '#type' => 'value',
    '#value' => $farm_asset->archived,
  );

  // Asset user id.
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $farm_asset->uid,
  );

  field_attach_form('farm_asset', $farm_asset, $form, $form_state);

  $form['actions'] = array(
    '#weight' => 100,
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#validate' => array('farm_asset_form_validate'),
    '#submit' => array('farm_asset_form_submit'),
  );

  // Show Delete button if allowed.
  $farm_asset_id = entity_id('farm_asset', $farm_asset);
  if (!empty($farm_asset_id) && farm_asset_access('delete', $farm_asset)) {

    // Get the destination query parameter. If it is the current path, change
    // to <front> (because the current path won't exist once the asset is
    // deleted).
    $destination = drupal_get_destination();
    if ($destination['destination'] == current_path()) {
      $destination['destination'] = '<front>';
    }
    $form['actions']['delete'] = array(
      '#type' => 'markup',
      '#markup' => l(t('Delete'), 'farm/asset/' . $farm_asset_id . '/delete', array('query' => $destination)),
    );
  }

  return $form;
}

/**
 * Asset validate handler.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function farm_asset_form_validate(array $form, array &$form_state) {

  // If the 'archived_boolean' checkbox has changed, update the asset.
  if ($form_state['values']['archived_boolean'] != $form['asset_status']['archived_boolean']['#default_value']) {

    // If it is checked, set the asset's archived date to current timestamp.
    if (!empty($form_state['values']['archived_boolean'])) {
      $form_state['values']['archived'] = REQUEST_TIME;
    }

    // Otherwise, set the asset's archived date to NULL.
    else {
      $form_state['values']['archived'] = NULL;
    }
  }
}

/**
 * Asset submit handler.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function farm_asset_form_submit(array $form, array &$form_state) {
  $farm_asset = $form_state['values']['farm_asset'];

  entity_form_submit_build_entity('farm_asset', $farm_asset, $form, $form_state);

  farm_asset_save($farm_asset);

  $farm_asset_uri = entity_uri('farm_asset', $farm_asset);

  $form_state['redirect'] = $farm_asset_uri['path'];

  drupal_set_message(t('Asset saved: <a href="@uri">%title</a>', array('@uri' => url($farm_asset_uri['path']), '%title' => entity_label('farm_asset', $farm_asset))));
}

/**
 * Delete confirmation form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 * @param FarmAsset $farm_asset
 *   The farm asset entity.
 *
 * @return array
 *   Returns a form array.
 */
function farm_asset_delete_form(array $form, array &$form_state, FarmAsset $farm_asset) {
  $form['farm_asset'] = array(
    '#type' => 'value',
    '#value' => $farm_asset,
  );
  // Always provide entity id in the same form key as in the entity edit form.
  $form['farm_asset_type_id'] = array(
    '#type' => 'value',
    '#value' => entity_id('farm_asset', $farm_asset),
  );
  $farm_asset_uri = entity_uri('farm_asset', $farm_asset);
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => entity_label('farm_asset', $farm_asset))),
    $farm_asset_uri['path'],
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Delete form submit handler.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function farm_asset_delete_form_submit(array $form, array &$form_state) {
  $farm_asset = $form_state['values']['farm_asset'];
  farm_asset_delete($farm_asset);

  drupal_set_message(t('%title was deleted.', array('%title' => entity_label('farm_asset', $farm_asset))));

  $form_state['redirect'] = '<front>';
}
