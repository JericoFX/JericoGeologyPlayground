"""Select a fixed single-pane recipe without changing earlier asset presets."""
def build_recipe(recipe):
    if recipe.get("preset")=="damp_wood_single_pane":
        from damp_wood import build_damp_window
        return build_damp_window(recipe)
    if str(recipe.get("preset","")).startswith("industrial_"):
        from industrial import build_industrial
        return build_industrial(recipe)
    if 'glazing' in recipe or recipe.get('preset')=='single_pane_fixed':
        from singlepane import build_singlepane
        return build_singlepane(recipe)
    from presets import build_preset
    return build_preset(recipe)
