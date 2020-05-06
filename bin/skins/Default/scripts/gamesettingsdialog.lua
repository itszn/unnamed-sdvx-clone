json = require "json"

local yScale = 0.0
local diagWidth = 600
local diagHeight = 400
local tabStroke = {start=0, stop=1}
local tabStrokeAnimation = {start=Animation:new(), stop=Animation:new()}
local settingsStrokeAnimation = {x=Animation:new(), y=Animation:new()}
local prevTab = -1
local prevSettingStroke = {x=0, y=0}
local settingStroke = {x=0, y=0}

function render(deltaTime, visible)
    if visible then
        yScale = clamp(yScale + deltaTime * 3, 0, 1)
    elseif yScale <= 0 then return
    else yScale = clamp(yScale - deltaTime * 3, 0, 1)
    end

    resX, resY = game.GetResolution()
    local scale = resY / 1080
    gfx.ResetTransform()
    gfx.Translate(math.floor(resX / 2), math.floor(resY / 2))
    gfx.Scale(scale, scale)
    gfx.Scale(1.0, smootherstep(0, 1, yScale))
    gfx.BeginPath()
    gfx.Rect(-diagWidth/2, -diagHeight/2, diagWidth, diagHeight)
    gfx.FillColor(50,50,50)
    gfx.Fill()
    gfx.FillColor(255,255,255)

    tabStroke.start = tabStrokeAnimation.start:tick(deltaTime)
    tabStroke.stop = tabStrokeAnimation.stop:tick(deltaTime)

    settingStroke.x = settingsStrokeAnimation.x:tick(deltaTime)
    settingStroke.y = settingsStrokeAnimation.y:tick(deltaTime)

    local tabBarHeight = 0
    local nextTabX = 5

    gfx.TextAlign(gfx.TEXT_ALIGN_TOP + gfx.TEXT_ALIGN_LEFT)
    gfx.FontSize(35)
    gfx.Save() --draw tab bar
    gfx.Translate(-diagWidth / 2, -diagHeight / 2)
    for ti, tab in ipairs(SettingsDiag.tabs) do
        local xmin,ymin, xmax,ymax = gfx.TextBounds(nextTabX, 5, tab.name)

        if ti == SettingsDiag.currentTab and SettingsDiag.currentTab ~= prevTab then 
            tabStrokeAnimation.start:restart(tabStroke.start, nextTabX, 0.1)
            tabStrokeAnimation.stop:restart(tabStroke.stop, xmax, 0.1)
        end
        tabBarHeight = math.max(tabBarHeight, ymax + 5)
        gfx.Text(tab.name, nextTabX, 5)
        nextTabX = xmax + 10
    end
    gfx.BeginPath()
    gfx.MoveTo(0, tabBarHeight)
    gfx.LineTo(diagWidth, tabBarHeight)
    gfx.StrokeWidth(2)
    gfx.StrokeColor(0,127,255)
    gfx.Stroke()
    gfx.BeginPath()
    gfx.MoveTo(tabStroke.start, tabBarHeight)
    gfx.LineTo(tabStroke.stop, tabBarHeight)
    gfx.StrokeColor(255, 127, 0)
    gfx.Stroke()
    gfx.Restore() --draw tab bar end

    gfx.FontSize(30)
    gfx.Save() --draw current tab
    gfx.Translate(-diagWidth / 2, -diagHeight / 2)
    gfx.Translate(5, tabBarHeight + 5)

    gfx.BeginPath()
    gfx.MoveTo(0, settingStroke.y)
    gfx.LineTo(settingStroke.x, settingStroke.y)
    gfx.StrokeWidth(2)
    gfx.StrokeColor(255, 127, 0)
    gfx.Stroke()

    local settingHeigt = 30
    local tab = SettingsDiag.tabs[SettingsDiag.currentTab]
    for si, setting in ipairs(tab.settings) do
        local disp = ""
        if setting.type == "enum" then
            disp = string.format("%s: %s", setting.name, setting.options[setting.value])
        elseif setting.type == "int" then
            disp = string.format("%s: %d", setting.name, setting.value)
        elseif setting.type == "float" then
            disp = string.format("%s: %.2f", setting.name, setting.value)
        else
            disp = string.format("%s:", setting.name)
            local xmin,ymin, xmax,ymax = gfx.TextBounds(0, 0, disp)
            gfx.BeginPath()
            gfx.Rect(xmax + 5, 7, 20,20)
            gfx.FillColor(255, 127, 0, setting.value and 255 or 0)
            gfx.StrokeColor(0,127,255)
            gfx.StrokeWidth(2)
            gfx.Fill()
            gfx.Stroke()
            gfx.FillColor(255,255,255)
        end
        gfx.Text(disp, 0 ,0)
        if si == SettingsDiag.currentSetting then
            local xmin,ymin, xmax,ymax = gfx.TextBounds(0, 0, setting.name .. ":")
            ymax = ymax + settingHeigt * (si - 1)
            if xmax ~= prevSettingStroke.x or ymax ~= prevSettingStroke.y then
                settingsStrokeAnimation.x:restart(settingStroke.x, xmax, 0.1)
                settingsStrokeAnimation.y:restart(settingStroke.y, ymax, 0.1)
            end

            prevSettingStroke.x = xmax
            prevSettingStroke.y = ymax
        end
        gfx.Translate(0, settingHeigt)
    end


    gfx.Restore() --draw current tab end
    prevTab = SettingsDiag.currentTab


end