json = require "json"


function render(deltaTime, visible)
    if not visible then return end
    gfx.ResetTransform()
    local tab = SettingsDiag.tabs[SettingsDiag.currentTab]
    local setting = tab.settings[SettingsDiag.currentSetting]
    gfx.TextAlign(0)
    gfx.Text(json.encode(setting), 5, 550)
end