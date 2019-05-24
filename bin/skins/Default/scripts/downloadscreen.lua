json = require "json"
local header = {}
header["user-agent"] = "unnamed_sdvx_clone"

local jacketFallback = gfx.CreateSkinImage("song_select/loading.png", 0)
local diffColors = {{50,50,127}, {50,127,50}, {127,50,50}, {127, 50, 127}}
local entryW = 770
local entryH = 320
local resX,resY = game.GetResolution()
local xCount = math.floor(resX / entryW)
local yCount = math.floor(resY / entryH)
local xOffset = (resX - xCount * entryW) / 2
local cursorPos = 0
local cursorPosX = 0
local cursorPosY = 0
local displayCursorPosX = 0
local displayCursorPosY = 0
local nextUrl = "https://ksm.dev/app/songs"
local loading = true
local downloaded = {}
local songs = {}
local cachepath = "skins/" .. game.GetSkin() .. "/nautica.json"
function addsong(song)
    if song.jacket_url ~= nil then
        song.jacket = gfx.LoadWebImageJob(song.jacket_url, jacketFallback, 250, 250)
    else
        song.jacket = jacketFallback
    end
    if downloaded[song.id] then
        song.status = "Downloaded"
    end
    table.insert(songs, song)
end
local yOffset = 0
local backgroundImage = gfx.CreateSkinImage("bg.png", 1);

dlcache = io.open(cachepath, "r")
if dlcache then
    downloaded = json.decode(dlcache:read("*all"))
    dlcache:close()
end
function encodeURI(str)
    if (str) then
        str = string.gsub(str, "\n", "\r\n")
        str = string.gsub(str, "([^%w ])",
            function (c) 
                local dontChange = "-/_:."
                for i = 1, #dontChange do
                    if c == dontChange:sub(i,i) then return c end
                end
                return string.format ("%%%02X", string.byte(c)) 
            end)
        str = string.gsub(str, " ", "%%20")
   end
   return str
end



function gotSongsCallback(response)
    if response.status ~= 200 then 
        error() 
        return 
    end
    local jsondata = json.decode(response.text)
    for i,song in ipairs(jsondata.data) do
        addsong(song)
    end
    nextUrl = jsondata.links.next
    loading = false
end

Http.GetAsync(nextUrl, header, gotSongsCallback)


function render_song(song, x,y)
    gfx.Save()    
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
    gfx.Translate(x,y)
    gfx.Scissor(0,0,750,300)
    gfx.BeginPath()
    gfx.FillColor(0,0,0,140)
    gfx.Rect(0,0,750,300)
    gfx.Fill()
    gfx.FillColor(255,255,255)
    gfx.FontSize(30)
    gfx.Text(song.title, 2,2)
    gfx.FontSize(24)
    gfx.Text(song.artist, 2,26)
    if song.jacket_url ~= nil and song.jacket == jacketFallback then
        song.jacket = gfx.LoadWebImageJob(song.jacket_url, jacketFallback, 250, 250)
    end
    gfx.BeginPath()
    gfx.ImageRect(0, 50, 250, 250, song.jacket, 1, 0)
    gfx.BeginPath()
    gfx.Rect(250,50,500,250)
    gfx.FillColor(55,55,55,128)
    gfx.Fill()
    for i, diff in ipairs(song.charts) do
        local col = diffColors[diff.difficulty]
        local diffY = 50 + 250/4 * (diff.difficulty - 1)
        gfx.BeginPath()
        
        gfx.Rect(250,diffY, 500, 250 / 4)
        gfx.FillColor(col[1], col[2], col[3])
        gfx.Fill()
        gfx.FillColor(255,255,255)
        gfx.FontSize(40)
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_MIDDLE)
        gfx.Text(string.format("%d Effected by %s", diff.level, diff.effector), 255, diffY + 250 / 8)
    end
    if downloaded[song.id] then
        gfx.BeginPath()
        gfx.Rect(0,0,750,300)
        gfx.FillColor(0,0,0,127)
        gfx.Fill()
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE)
        gfx.FontSize(60)
        gfx.FillColor(255,255,255)
        gfx.Text(downloaded[song.id], 375, 150)
    end
    gfx.ResetScissor()
    gfx.Restore()
end

function load_more()
    if nextUrl ~= nil and not loading then
        Http.GetAsync(nextUrl, header, gotSongsCallback)
        loading = true
    end
end

function render_cursor()
    local x = displayCursorPosX * entryW
    local y = displayCursorPosY * entryH
    gfx.BeginPath()
    gfx.Rect(x,y,750,300)
    gfx.StrokeColor(255,128,0)
    gfx.StrokeWidth(5)
    gfx.Stroke()
end

function render(deltaTime)
    gfx.ImageRect(0, 0, resX, resY, backgroundImage, 1, 0);
    gfx.LoadSkinFont("NotoSans-Regular.ttf");
    displayCursorPosX = displayCursorPosX - (displayCursorPosX - cursorPosX) * deltaTime * 10
    displayCursorPosY = displayCursorPosY - (displayCursorPosY - cursorPosY) * deltaTime * 10
    if displayCursorPosY - yOffset > yCount - 1 then --scrolling down
        yOffset = yOffset - (yOffset - displayCursorPosY) - yCount + 1
    elseif displayCursorPosY - yOffset < 0 then
        yOffset = yOffset - (yOffset - displayCursorPosY)
    end
    gfx.Translate(xOffset, 50 - yOffset * entryH)
    for i, song in ipairs(songs) do
        if math.abs(cursorPos - i) <= xCount * yCount + xCount then
            i = i - 1
            local x = entryW * (i % xCount)
            local y = math.floor(i / xCount) * entryH
            render_song(song, x, y)
            if math.abs(#songs - i) < 4 then load_more() end
        end
    end
    render_cursor()
end

function archive_callback(entries, id)
    game.Log("Listing entries for " .. id, 0)
    local songsfolder = dlScreen.GetSongsPath()
    res = {}
    for i, entry in ipairs(entries) do
        game.Log(entry, 0)
        res[entry] = songsfolder .. "/" .. entry
    end
    downloaded[id] = "Downloaded"
    return res
end

function button_pressed(button)
    if button == game.BUTTON_STA then
        local song = songs[cursorPos + 1]
        if song == nil then return end
        dlScreen.DownloadArchive(encodeURI(song.cdn_download_url), header, song.id, archive_callback)
        downloaded[song.id] = "Downloading..."
    end
end

function key_pressed(key)
    if key == 27 then --escape pressed
        dlcache = io.open(cachepath, "w")
        dlcache:write(json.encode(downloaded))
        dlcache:close()
        dlScreen.Exit() 
    end
end

function advance_selection(steps)
    cursorPos = (cursorPos + steps) % #songs
    cursorPosX = cursorPos % xCount
    cursorPosY = math.floor(cursorPos / xCount)
    if cursorPos > #songs - 6 then
        load_more()
    end
end