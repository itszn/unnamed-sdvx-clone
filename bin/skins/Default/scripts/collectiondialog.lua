options = {}
yscale = 0.0
selectedIndex = 0
resx, resy = game.GetResolution()
width = 500
titleText = ""

function make_option(name)
    return function()
        menu.Confirm(name)
    end
end

function open()
    yscale = 0.0
    options = {}
    selectedIndex = 0
    resx, resy = game.GetResolution()
    
    index = 1
    if #dialog.collections == 0 then
        options[index] = {"Favourites", make_option("Favourites")}
    end
    
    for i,name in ipairs(dialog.collections) do
        options[i] = {name, make_option(name)}
    end
    table.insert(options, {"New Collection", menu.ChangeState})
    table.insert(options, {"Cancel", menu.Cancel})
    
    gfx.FontFace("fallback")
    gfx.FontSize(50)
    titleText = string.format("Add %s to collection:", dialog.title)
    xmi,ymi,xma,yma = gfx.TextBounds(0,0, titleText)
    width = xma - xmi + 50
    width = math.max(500, width)
end

function render(deltaTime)
    if dialog.closing then
        yscale = math.min(yscale - deltaTime * 4, 1.0)
    else
        yscale = math.min(yscale + deltaTime * 4, 1.0)
    end
    gfx.Translate(resx / 2, resy / 2)
    gfx.Scale(1.0, yscale)
    gfx.BeginPath()
    gfx.Rect(-width/2, -250, width, 500)
    gfx.FillColor(50,50,50)
    gfx.Fill()
    
    gfx.BeginPath()
    gfx.FillColor(255,255,255)
    gfx.FontFace("fallback")
    gfx.FontSize(50)
    gfx.TextAlign(gfx.TEXT_ALIGN_TOP + gfx.TEXT_ALIGN_CENTER)
    gfx.Text(titleText, 0, -240)

    if dialog.isTextEntry then
        gfx.TextAlign(gfx.TEXT_ALIGN_MIDDLE + gfx.TEXT_ALIGN_CENTER)
        gfx.BeginPath()
        gfx.Rect(-width/2 + 20, -30, width - 40, 60)
        gfx.FillColor(25,25,25)
        gfx.Fill()
        gfx.BeginPath()
        gfx.FillColor(255,255,255)
        gfx.Text(dialog.newName, 0, 0)
    else
        gfx.TextAlign(gfx.TEXT_ALIGN_MIDDLE + gfx.TEXT_ALIGN_LEFT)
        for i, option in ipairs(options) do
            gfx.Text(option[1], 40 - width/2, 60 * ((i-1) - selectedIndex))
        end
        gfx.BeginPath()
        gfx.MoveTo(20 - width/2, -10)
        gfx.LineTo(30 - width/2, 0)
        gfx.LineTo(20 - width/2, 10)
        gfx.FillColor(255,255,255)
        gfx.Fill()
    end
    if dialog.closing == true and yscale <= 0.0 then
        return false
    else
        return true
    end
end

function button_pressed(button)
    if button == game.BUTTON_BCK then
        menu.Cancel()
    elseif button == game.BUTTON_STA then
        options[selectedIndex+1][2]()
    end
end

function advance_selection(value)
    selectedIndex = (selectedIndex + value) % #options
end