local mposx = 0;
local mposy = 0;
local hovered = nil;
local buttonWidth = 250;
local buttonHeight = 75;
local buttonBorder = 2;
local label = -1;

local gr_r, gr_g, gr_b, gr_a = game.GetSkinSetting("col_test")
gfx.GradientColors(0,127,255,255,0,128,255,0)
local gradient = gfx.LinearGradient(0,0,0,1)
local bgPattern = gfx.CreateSkinImage("bg_pattern.png", gfx.IMAGE_REPEATX + gfx.IMAGE_REPEATY)
local bgAngle = 0.5
local bgPaint = gfx.ImagePattern(0,0, 256,256, bgAngle, bgPattern, 1.0)
local bgPatternTimer = 0

view_update = function()
    if package.config:sub(1,1) == '\\' then --windows
        updateUrl, updateVersion = game.UpdateAvailable()
        os.execute("start " .. updateUrl)
    else --unix
        --TODO: Mac solution
        os.execute("xdg-open " .. updateUrl)
    end
end

mouse_clipped = function(x,y,w,h)
    return mposx > x and mposy > y and mposx < x+w and mposy < y+h;
end;

draw_button = function(name, x, y, hoverindex)
    local rx = x - (buttonWidth / 2);
    local ty = y - (buttonHeight / 2);
    gfx.BeginPath();
    gfx.FillColor(0,128,255);
    if mouse_clipped(rx,ty, buttonWidth, buttonHeight) then
       hovered = hoverindex;
       gfx.FillColor(255,128,0);
    end
    gfx.Rect(rx - buttonBorder,
        ty - buttonBorder,
        buttonWidth + (buttonBorder * 2),
        buttonHeight + (buttonBorder * 2));
    gfx.Fill();
    gfx.BeginPath();
    gfx.FillColor(40,40,40);
    gfx.Rect(rx, ty, buttonWidth, buttonHeight);
    gfx.Fill();
    gfx.BeginPath();
    gfx.FillColor(255,255,255);
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    gfx.Text(name, x, y);
end;

function updateGradient()
	gr_r, gr_g, gr_b, gr_a = game.GetSkinSetting("col_test")
	if gr_r == nil then return end
	gfx.GradientColors(gr_r,gr_g,gr_b,gr_a,0,128,255,0)
	gradient = gfx.LinearGradient(0,0,0,1)
end

function updatePattern(dt)
	bgPatternTimer = (bgPatternTimer + dt) % 1.0
	local bgx = math.cos(bgAngle) * (bgPatternTimer * 256)
	local bgy = math.sin(bgAngle) * (bgPatternTimer * 256)
	gfx.UpdateImagePattern(bgPaint, bgx, bgy, 256, 256, bgAngle, 1.0)
end

render = function(deltaTime)
	updateGradient()
	updatePattern(deltaTime)
    resx,resy = game.GetResolution();
    mposx,mposy = game.GetMousePos();
    gfx.Scale(resx, resy / 3)
    gfx.Rect(0,0,1,1)
    gfx.FillPaint(gradient)
    gfx.Fill()
    gfx.ResetTransform()
    gfx.BeginPath()
	gfx.Scale(0.5,0.5)
	gfx.Rect(0,0,resx * 2,resy * 2)
	gfx.GlobalCompositeOperation(gfx.BLEND_OP_DESTINATION_IN)
    gfx.FillPaint(bgPaint)
    gfx.Fill()
	gfx.ResetTransform()
	gfx.BeginPath()
	gfx.GlobalCompositeOperation(gfx.BLEND_OP_SOURCE_OVER)
    buttonY = resy / 2;
    hovered = nil;
    gfx.LoadSkinFont("NotoSans-Regular.ttf");
    draw_button("Start", resx / 2, buttonY, Menu.Start);
    buttonY = buttonY + 100;
    draw_button("Settings", resx / 2, buttonY, Menu.Settings);
    buttonY = buttonY + 100;
    draw_button("Exit", resx / 2, buttonY, Menu.Exit);
    gfx.BeginPath();
    gfx.FillColor(255,255,255);
    gfx.FontSize(120);
    if label == -1 then
        label = gfx.CreateLabel("unnamed_sdvx_clone", 120, 0);
    end
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
    gfx.DrawLabel(label, resx / 2, resy / 2 - 200, resx-40);
    updateUrl, updateVersion = game.UpdateAvailable()
    if updateUrl then
       gfx.BeginPath()
       gfx.TextAlign(gfx.TEXT_ALIGN_BOTTOM + gfx.TEXT_ALIGN_LEFT)
       gfx.FontSize(30)
       gfx.Text(string.format("Version %s is now available", updateVersion), 5, resy - buttonHeight - 10)
       draw_button("View", buttonWidth / 2 + 5, resy - buttonHeight / 2 - 5, view_update);
    end
end;

mouse_pressed = function(button)
    if hovered then
        hovered()
    end
    return 0
end
