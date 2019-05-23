json = require "json"
local header = {}
header["user-agent"] = "unnamed_sdvx_clone"

local jacketFallback = gfx.CreateSkinImage("song_select/loading.png", 0)
local diffColors = {{50,50,127}, {50,127,50}, {127,50,50}, {127, 50, 127}}
local cursorPos = 0
local cursorPosX = 0
local cursorPosY = 0
local displayCursorPosX = 0
local displayCursorPosY = 0
local nextUrl = "https://ksm.dev/app/songs"
local loading = true

local songs = {}
function addsong(song)
    song.jacket = gfx.LoadWebImageJob(song.jacket_url, jacketFallback, 250, 250)
    table.insert(songs, song)
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
    gfx.FillColor(255,255,255)
    gfx.Translate(x,y)
    gfx.FontSize(25)
    gfx.Text(song.title, 0,0)
    gfx.FontSize(20)
    gfx.Text(song.artist, 0,30)
    if song.jacket == jacketFallback then
        song.jacket = gfx.LoadWebImageJob(song.jacket_url, jacketFallback, 250, 250)
    end
    gfx.BeginPath()
    gfx.ImageRect(0, 50, 250, 250, song.jacket, 1, 0)
    gfx.BeginPath()
    gfx.Rect(250,50,500,250)
    gfx.FillColor(55,55,55)
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
    gfx.Restore()
end

function render_cursor()
    local x = displayCursorPosX * 800
    local y = displayCursorPosY * 400
    gfx.BeginPath()
    gfx.Rect(x-5,y-5,760,310)
    gfx.FillColor(0,0,200)
    gfx.Fill()
end

function render(deltaTime)
    
    gfx.LoadSkinFont("NotoSans-Regular.ttf");
    displayCursorPosX = displayCursorPosX - (displayCursorPosX - cursorPosX) * deltaTime * 10
    displayCursorPosY = displayCursorPosY - (displayCursorPosY - cursorPosY) * deltaTime * 10
    gfx.Translate(50, 50 - displayCursorPosY * 400)
    render_cursor()
    for i, song in ipairs(songs) do
        if math.abs(cursorPos - i) <= 10 then
            i = i - 1
            local x = 800 * (i % 2)
            local y = (i - (i % 2)) * 200
            render_song(song, x, y)
        
        end
    end
    
end

function key_pressed(key)
    if key == 27 then dlScreen.Exit() end --escape pressed
end

function advance_selection(steps)
    cursorPos = (cursorPos + steps) % #songs
    cursorPosX = cursorPos % 2
    cursorPosY = math.floor(cursorPos / 2)
    if cursorPos > #songs - 6 and nextUrl ~= nil and not loading then
        Http.GetAsync(nextUrl, header, gotSongsCallback)
        loading = true
    end
    
end