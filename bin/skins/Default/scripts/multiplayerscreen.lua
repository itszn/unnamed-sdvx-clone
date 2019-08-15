json = require "json"

local resX,resY = game.GetResolution()

local mposx = 0;
local mposy = 0;
local hovered = nil;
local buttonWidth = resX*(3/4);
local buttonHeight = 75;
local buttonBorder = 2;

local SERVER = game.GetSkinSetting("multiplayer_server")

local loading = true;
local rooms = {};
local lobby_users = {};
local selected_room = nil;
local user_id = nil;
local jacket = 0;
local all_ready;
local user_ready;
local hard_mode = false;
local rotate_host = false;
local start_game_soon = false;
local host = nil;
local missing_song = false;

local did_exit = false;

local grades = {
    {["max"] = 6999999, ["image"] = gfx.CreateSkinImage("score/D.png", 0)},
    {["max"] = 7999999, ["image"] = gfx.CreateSkinImage("score/C.png", 0)},
    {["max"] = 8699999, ["image"] = gfx.CreateSkinImage("score/B.png", 0)},
    {["max"] = 8999999, ["image"] = gfx.CreateSkinImage("score/A.png", 0)},
    {["max"] = 9299999, ["image"] = gfx.CreateSkinImage("score/A+.png", 0)},
    {["max"] = 9499999, ["image"] = gfx.CreateSkinImage("score/AA.png", 0)},
    {["max"] = 9699999, ["image"] = gfx.CreateSkinImage("score/AA+.png", 0)},
    {["max"] = 9799999, ["image"] = gfx.CreateSkinImage("score/AAA.png", 0)},
    {["max"] = 9899999, ["image"] = gfx.CreateSkinImage("score/AAA+.png", 0)},
    {["max"] = 99999999, ["image"] = gfx.CreateSkinImage("score/S.png", 0)}
  }


local user_name_key = game.GetSkinSetting('multi.user_name_key')
if user_name_key == nil then
  user_name_key = 'nick'
end
local name = game.GetSkinSetting(user_name_key)
if name == nil or name == '' then
    name = 'Guest'
end

local normal_font = game.GetSkinSetting('multi.normal_font')
if normal_font == nil then
    normal_font = 'NotoSans-Regular.ttf'
end
local mono_font = game.GetSkinSetting('multi.mono_font')
if mono_font == nil then
    mono_font = 'NovaMono.ttf'
end

local SERVER = game.GetSkinSetting("multi.server")





mouse_clipped = function(x,y,w,h)
    return mposx > x and mposy > y and mposx < x+w and mposy < y+h;
end;

draw_room = function(name, x, y, hoverindex)
    local buttonWidth = resX*(3/4);
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

draw_button = function(name, x, y, buttonWidth, hoverindex)
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

draw_checkbox = function(text, x, y, hoverindex, current, can_click)
    local rx = x - (buttonWidth / 2);
    local ty = y - (buttonHeight / 2);
    gfx.BeginPath();
    
    gfx.FillColor(255,255,255);
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(35);
    gfx.Text(text, x, y)

    local xmin,ymin,xmax,ymax = gfx.TextBounds(x, y, text);

    local sx = xmin - 40;
    local sy = y - 15;

    gfx.StrokeColor(0,128,255);
    if can_click and (mouse_clipped(sx, sy, 31, 30) or mouse_clipped(xmin-10, ymin, xmax-xmin, ymax-ymin)) then
        hovered = hoverindex;
        gfx.StrokeColor(255,128,0);
     end

    gfx.Rect(sx, y - 15, 30, 30)
    gfx.StrokeWidth(2)
    gfx.Stroke()

    if current then
        -- Draw checkmark
        gfx.BeginPath();
        gfx.MoveTo(sx+5, sy+10);
        gfx.LineTo(sx+15, y+5);
        gfx.LineTo(sx+35, y-15);
        gfx.StrokeWidth(5)
        gfx.StrokeColor(0,255,0);
        gfx.Stroke()

    end
end;

local userHeight = 100

draw_user = function(user, x, y, rank)
    local buttonWidth = resX*(3/8);
    local buttonHeight = userHeight;
    local rx = x - (buttonWidth / 2);
    local ty = y - (buttonHeight / 2);
    gfx.BeginPath();
    gfx.FillColor(256,128,255);

    
    if user_id == host and mouse_clipped(rx,ty, buttonWidth, buttonHeight) then
        hovered = function()
            change_host(user)
        end
        gfx.FillColor(255,255,255);
     end

    gfx.Rect(rx - buttonBorder,
        ty - buttonBorder,
        buttonWidth + (buttonBorder * 2),
        buttonHeight + (buttonBorder * 2));
    gfx.Fill();
    gfx.BeginPath();
    if host == user.id then
        gfx.FillColor(80,0,0);
    else
        gfx.FillColor(0,0,40);
    end
    gfx.Rect(rx, ty, buttonWidth, buttonHeight);
    gfx.Fill();
    gfx.BeginPath();
    gfx.FillColor(255,255,255);
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    local name = user.name;
    if user.id == user_id then
        name = name
    end
    if user.id == host then
        name = name..' (host)'
    elseif user.missing_map then
        name = name..' (NO CHART)'
    elseif user.ready then
        name = name..' (ready)'
    end
    if user.score ~= nil then
        name = '#'..rank..' '..name
    end
    first_y = y - 28
    second_y = y + 28
    gfx.Text(name, x - buttonWidth/2 + 5, first_y);
    if user.score ~= nil then
        gfx.FillColor(255,255,0)
        gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_MIDDLE);
        local combo_text = '  '..user.combo..'x'
        gfx.Text(combo_text, x+buttonWidth/2 - 5, second_y);
        local xmin,ymin,xmax,ymax = gfx.TextBounds(x+buttonWidth/2 - 5, second_y, combo_text);

        
        local score_text = '  '..string.format("%08d",user.score);
        gfx.FillColor(255,255,255)
        gfx.Text(score_text, xmin, second_y);
        xmin,ymin,xmax,ymax = gfx.TextBounds(xmin, second_y, score_text);

        if user.grade == nil then
            for i,v in ipairs(grades) do
                if v.max > user.score then
                    user.grade = v.image
                    break
                end
            end
        end

        gfx.BeginPath()
        local iw, ih = gfx.ImageSize(user.grade)
        local iar = iw/ih
        local grade_height = buttonHeight/2 - 20
        gfx.ImageRect(xmin - iar * grade_height, second_y - buttonHeight/4 + 10 , iar * grade_height, grade_height, user.grade, 1, 0)
    end
end;

function render_loading()
    if not loading then return end
    gfx.Save()
    gfx.ResetTransform()
    gfx.BeginPath()
    gfx.MoveTo(resX, resY)
    gfx.LineTo(resX - 350, resY)
    gfx.LineTo(resX - 300, resY - 50)
    gfx.LineTo(resX, resY - 50)
    gfx.ClosePath()
    gfx.FillColor(33,33,33)
    gfx.Fill()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT, gfx.TEXT_ALIGN_BOTTOM)
    gfx.FontSize(70)
    gfx.Text("LOADING...", resX - 20, resY - 3)
    gfx.Restore()
end

function render_info()
    gfx.Save()
    gfx.ResetTransform()
    gfx.BeginPath()
    gfx.MoveTo(0, resY)
    gfx.LineTo(550, resY)
    gfx.LineTo(500, resY - 65)
    gfx.LineTo(0, resY - 65)
    gfx.ClosePath()
    gfx.FillColor(33,33,33)
    gfx.Fill()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT, gfx.TEXT_ALIGN_BOTTOM)
    gfx.FontSize(70)
    gfx.Text("Multiplayer", 3, resY - 15)
    local xmin,ymin,xmax,ymax = gfx.TextBounds(3, resY - 3, "Multiplayer")
    gfx.FontSize(20)
    gfx.Text('v0.01', xmax + 13, resY - 15)
    --gfx.Text('Server: '..'', xmax + 13, resY - 15)
    gfx.Restore()
end

render = function(deltaTime)
    resx,resy = game.GetResolution();
    mposx,mposy = game.GetMousePos();
    
    hovered = nil;

    gfx.LoadSkinFont(normal_font);

    -- Room Listing View
    if selected_room == nil then
        gfx.FillColor(255,255,255)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER, gfx.TEXT_ALIGN_BOTTOM)
        gfx.FontSize(70)
        gfx.Text("Multiplayer Rooms", resx/2, 100)

        buttonY = 175;

        for i, room in ipairs(rooms) do
            local l_room = room
            local status = room.current..'/'..room.max
            if room.ingame then
                status = status..' (In Game)'
            end
            draw_room(room.name .. ':  '.. status, resx / 2, buttonY, function ()
                join_room(l_room)
            end);
            buttonY = buttonY + 100
        end
    
    -- Room Lobby View
    else
        gfx.FillColor(255,255,255)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER, gfx.TEXT_ALIGN_BOTTOM)
        gfx.FontSize(70)
        gfx.Text("Room "..selected_room.name, resx/2, 100)
        gfx.Text("Users", resx/4, 200)

        buttonY = 275;
        for i, user in ipairs(lobby_users) do
            draw_user(user, resx / 4, buttonY, i)
            buttonY = buttonY + 75
        end
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
        gfx.FillColor(255,255,255)

        gfx.FontSize(60)
        gfx.Text("Selected Song:", resx*3/4, 200)
        gfx.FontSize(40)
        if selected_song == nil then
            if host == user_id then
                gfx.Text("Select song:", resx*3/4, 275)
            else
                if missing_song then
                    gfx.Text("Missing song!!!!", resx*3/4, 275)
                else
                    gfx.Text("Host is selecting song", resx*3/4, 275)
                end
            end
            if jacket == 0 then
                jacket = gfx.CreateSkinImage("song_select/loading.png", 0)
            end
        else
            gfx.Text(selected_song.title..' ['..selected_song.level..']', resx*3/4, 275)
            if selected_song.jacket == nil then
                selected_song.jacket = gfx.CreateImage(selected_song.jacketPath, 0)
                jacket = selected_song.jacket
            end
        end
        gfx.Save()
        gfx.BeginPath()
        local size = math.min(resx/2, resy/2);
        gfx.Translate(resx*3/4, 325+size/2)
        gfx.ImageRect(-size/2,-size/2,size,size,jacket,1,0)
        
        if mouse_clipped(resx*3/4-size/2, 325, size,size) and host == user_id then
            hovered = function() 
                missing_song = false
                mpScreen.SelectSong()
            end
        end
        gfx.Restore()
        if start_game_soon then
            draw_button("Game starting...", resx*3/4, 375+size, 600, function() end);
        else
            if host == user_id then
                if selected_song == nil or not selected_song.self_picked then
                    draw_button("Select song", resx*3/4, 375+size, 600, function() 
                        missing_song = false
                        mpScreen.SelectSong()
                    end);
                elseif user_ready and all_ready then
                    draw_button("Start game", resx*3/4, 375+size, 600, start_game)
                elseif user_ready and not all_ready then
                    draw_button("Waiting for others", resx*3/4, 375+size, 600, function() end)
                else
                    draw_button("Ready", resx*3/4, 375+size, 600, ready_up);
                end
            elseif host == nil then
                draw_button("Waiting for game to end", resx*3/4, 375+size, 600, function() end);
            elseif missing_song then
                draw_button("Missing Song!", resx*3/4, 375+size, 600, function() end);
            elseif selected_song ~= nil then
                if user_ready then
                    draw_button("Cancel", resx*3/4, 375+size, 600, ready_up);
                else
                    draw_button("Ready", resx*3/4, 375+size, 600, ready_up);
                end
            else
                draw_button("Waiting for host", resx*3/4, 375+size, 600, function() end);
            end
        end

        draw_checkbox("Excessive Gauge", resx*3/4 - 150 + 30, 375+size + 70, toggle_hard, hard_mode, host == user_id and not start_game_soon)
        draw_checkbox("Rotate Host", resx*3/4 + 150 + 20, 375+size + 70, toggle_rotate, do_rotate, host == user_id and not start_game_soon)
    end
    render_loading();
    render_info();
end

-- Ready up to play
function ready_up()
    Tcp.SendLine(json.encode({topic="user.ready.toggle"}))
end

-- Toggle hard gauage
function toggle_hard()
    Tcp.SendLine(json.encode({topic="room.option.hard.toggle"}))
end

-- Toggle host rotation
function toggle_rotate()
    Tcp.SendLine(json.encode({topic="room.option.rotation.toggle"}))
end

-- Change lobby host
function change_host(user)
    Tcp.SendLine(json.encode({topic="room.host.set", host=user.id}))
end

-- Process the response from the server when the game is started
function start_game_callback(response)
    if (response.status ~= 200) then 
        selected_room = nil
        error() 
        return 
    end
    loading = false
    start_game_soon = true;

    local jsondata = json.decode(response.text)
    mpScreen.UpdateTime(jsondata.left)
    if jsondata.left == 0 then
        update_tick()
        return
    end
end

-- Tell the server to start the game
function start_game()
    selected_song.self_picked = false
    if (selected_song == nil) then
        return
    end
    if (start_game_soon) then
        return
    end
    
    Tcp.SendLine(json.encode({topic="room.game.start"}))
end



-- Join a given room
function join_room(room)
    Tcp.SendLine(json.encode({topic="server.room.join", id=room.id}))
end

-- Handle button presses to advance the UI
button_pressed = function(button)
    if button == game.BUTTON_STA then
        if start_game_soon then
            return
        end
        if host == user_id then
            if selected_song and selected_song.self_picked then
                if all_ready then
                    start_game()
                elseif not user_ready then
                    ready_up()
                end
            else
                missing_song = false
                mpScreen.SelectSong()
            end
        else
            ready_up()
        end
    end
end

-- Handle the escape key around the UI
function key_pressed(key)
    if key == 27 then --escape pressed
        if selected_room == nil then
            did_exit = true;
            mpScreen.Exit();
            return
        end

        -- Reset room data
        selected_room = nil;
        rooms = {};
        selected_song = nil;
        loading = true;
        jacket = 0;
        Tcp.SendLine(json.encode({topic="room.leave"}));
    end

end

-- Handle mouse clicks in the UI
mouse_pressed = function(button)
    if hovered then
        hovered()
    end
    return 0
end

function init_tcp()
-- Update the list of rooms as well as get user_id for the client
Tcp.SetTopicHandler("server.rooms", function(data)
    loading = false
    user_id = data.userid

    rooms = {}
    for i, room in ipairs(data.rooms) do
        table.insert(rooms, room)
    end
end)

Tcp.SetTopicHandler("server.room.joined", function(data)
    selected_room = data.room
end)

-- Update the current lobby
Tcp.SetTopicHandler("room.update", function(data)
    -- Update the users in the lobby
    lobby_users = {}
    all_ready = true
    for i, user in ipairs(data.users) do
        table.insert(lobby_users, user)
        if user.id == user_id then
            user_ready = user.ready
        end
        if not user.ready then
            all_ready = false
        end
    end

    host = data.host
    hard_mode = data.hard_mode
    do_rotate = data.do_rotate
    start_game_soon = data.start_soon

end)
end

-- Update the rooms


-- Perform pull requests or start the game
update_tick_bad = function()
    if start_game_soon then
        mpScreen.UpdateTime(0)
        mpScreen.StartGame(hard_mode)
        return
    end
    if loading then
        return
    end
end