"""Select a fixed single-pane recipe without changing earlier asset presets."""
def build_recipe(recipe):
    if 'glazing' in recipe or recipe.get('preset')=='single_pane_fixed':
        from singlepane import build_singlepane
        return build_singlepane(recipe)
    from presets import build_preset
    return build_preset(recipe)
