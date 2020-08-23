local jacketImg = nil

local desw = 800
local desh = 800

local moveX = 0
local moveY = 0

local currResX = 0
local currResY = 0

local scale = 1

local gradeImg = nil;
local badgeImg = nil;
local gradeAR = 1 --grade aspect ratio

local diffNames = {"NOV", "ADV", "EXH", "INF"}
local backgroundImage = gfx.CreateSkinImage("bg.png", 1);
game.LoadSkinSample("applause")
local played = false
local shotTimer = 0;
local shotPath = "";
game.LoadSkinSample("shutter")
local highScores = nil

local highestScore = 0

local hasHitStat = false
local hitHistogram = {}
local hitDeltaScale = 1.5
local hitMinDelta = 0
local hitMaxDelta = 0

local hitWindowPerfect = 46
local hitWindowGood = 92

local showStatsHit = false
local prevShowStats = false
local clearText = ""

function getTextScale(txt, max_width)
    local x1, y1, x2, y2 = gfx.TextBounds(0, 0, txt)
    if x2 < max_width then
        return 1
    else
        return max_width / x2
    end
end

function drawScaledText(txt, x, y, max_width)
    gfx.Save()
    
    local text_scale = getTextScale(txt, max_width)
    
    gfx.Translate(x, y)
    gfx.Scale(text_scale, 1)
    gfx.Text(txt, 0, 0)
    
    gfx.Restore()
end

function drawLine(x1,y1,x2,y2,w,r,g,b)
    gfx.BeginPath()
    gfx.MoveTo(x1,y1)
    gfx.LineTo(x2,y2)
    gfx.StrokeColor(r,g,b)
    gfx.StrokeWidth(w)
    gfx.Stroke()
end

function result_set()
    highScores = { }
    currentAdded = false
    if result.uid == nil then --local scores
        for i,s in ipairs(result.highScores) do
            newScore = { }
            if currentAdded == false and result.score > s.score then
                newScore.score = string.format("%08d", result.score)
                newScore.color = {255, 127, 0}
                newScore.subtext = "Now"
                newScore.xoff = 0
                table.insert(highScores, newScore)
                newScore = { }
                currentAdded = true
            end
            newScore.score = string.format("%08d", s.score)
            newScore.color = {0, 127, 255}
            newScore.xoff = 0
            if s.timestamp > 0 then
                newScore.subtext = os.date("%Y-%m-%d %H:%M:%S", s.timestamp)
            else 
                newScore.subtext = ""
            end
            
            if highestScore < s.score then
                highestScore = s.score
            end
            
            table.insert(highScores, newScore)
        end

        if currentAdded == false then
            newScore = { }
            newScore.score = string.format("%08d", result.score)
            newScore.color = {255, 127, 0}
            newScore.subtext = "Now"
            newScore.xoff = 0
            table.insert(highScores, newScore)
            newScore = { }
            currentAdded = true
        end
    else --multi scores
        for i,s in ipairs(result.highScores) do
            newScore = { }
            if s.uid == result.uid then 
                newScore.color = {255, 127, 0}
            else
                newScore.color = {0, 127, 255}
            end

            if result.displayIndex + 1 == i then
                newScore.xoff = -20
            else
                newScore.xoff = 0
            end

            newScore.score = string.format("%08d", s.score)
            newScore.subtext = s.name
            
            if highestScore < s.score then
                highestScore = s.score
            end
            
            table.insert(highScores, newScore)
        end
    end
    
    if result.jacketPath ~= nil and result.jacketPath ~= "" then
        jacketImg = gfx.CreateImage(result.jacketPath, 0)
    end
    
    gradeImg = gfx.CreateSkinImage(string.format("score/%s.png", result.grade), 0)
    if gradeImg ~= nil then
        local gradew, gradeh = gfx.ImageSize(gradeImg)
        gradeAR = gradew / gradeh
    end
    
    do
        local path = nil
        if result.badge == 1 then path = "badges/played.png"
        elseif result.badge == 2 then path = "badges/clear.png"
        elseif result.badge == 3 then path = "badges/hard-clear.png"
        elseif result.badge == 4 then path = "badges/full-combo.png"
        elseif result.badge == 5 then path = "badges/perfect.png"
        end
        
        if path ~= nil then
            badgeImg = gfx.CreateSkinImage(path, 0)
        end
    end
    
    if result.autoplay then clearText = "AUTOPLAY"
    elseif result.hitWindow ~= nil and result.hitWindow.type == 0 then clearText = "EXPAND JUDGE"
    elseif result.badge == 0 then clearText = "NOT SAVED"
    elseif result.badge == 1 then clearText = "PLAYED"
    elseif result.badge == 2 then clearText = "CLEAR"
    elseif result.badge == 3 then clearText = "HARD CLEAR"
    elseif result.badge == 4 then clearText = "FULL COMBO"
    elseif result.badge == 5 then clearText = "PERFECT"
    else clearText = ""
    end
    
    hasHitStat = result.noteHitStats ~= nil and #result.noteHitStats > 0
    
    if result.hitWindow ~= nil then
        hitWindowPerfect = result.hitWindow.perfect
        hitWindowGood = result.hitWindow.good
    end
    
    if hasHitStat then
        hitHistogram = {}
        for i = 1, #result.noteHitStats do
            local hitStat = result.noteHitStats[i]
            if hitStat.rating == 1 or hitStat.rating == 2 then
                if hitHistogram[hitStat.delta] == nil then hitHistogram[hitStat.delta] = 0 end
                hitHistogram[hitStat.delta] = hitHistogram[hitStat.delta] + 1
                
                if hitStat.delta < hitMinDelta then hitMinDelta = hitStat.delta end
                if hitStat.delta > hitMaxDelta then hitMaxDelta = hitStat.delta end
            end
        end
    end
end

draw_shotnotif = function(x,y)
    gfx.LoadSkinFont("NotoSans-Regular.ttf")
    gfx.Save()
    gfx.Translate(x,y)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
    gfx.BeginPath()
    gfx.Rect(0,0,200,40)
    gfx.FillColor(30,30,30)
    gfx.StrokeColor(255,128,0)
    gfx.Fill()
    gfx.Stroke()
    gfx.FillColor(255,255,255)
    gfx.FontSize(15)
    gfx.Text("Screenshot saved to:", 3,5)
    gfx.Text(shotPath, 3,20)
    gfx.Restore()
end

draw_stat = function(x,y,w,h, name, value, format,r,g,b)
    gfx.Save()
    gfx.Translate(x,y)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
    gfx.FontSize(h)
    gfx.Text(name .. ":",0, 0)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_TOP)
    gfx.Text(string.format(format, value),w, 0)
    gfx.BeginPath()
    gfx.MoveTo(0,h)
    gfx.LineTo(w,h)
    if r then gfx.StrokeColor(r,g,b) 
    else gfx.StrokeColor(200,200,200) end
    gfx.StrokeWidth(1)
    gfx.Stroke()
    gfx.Restore()
    return y + h + 5
end

draw_score = function(score, x, y, w, h, pre)
    local center = x + w * 0.55
    local prefix = ""
    if pre ~= nil then prefix = pre end

    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.BeginPath()
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT)
    gfx.FontSize(h)
    gfx.Text(string.format("%s%04d", prefix, score // 10000), center-h/70, y)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
    gfx.FontSize(h*0.75)
    gfx.Text(string.format("%04d", score % 10000), center+h/70, y)
end

draw_highscores = function()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
    gfx.LoadSkinFont("NotoSans-Regular.ttf")
    gfx.FontSize(30)
    gfx.Text("Highscores:",510,30)
    gfx.StrokeWidth(1)
    for i,s in ipairs(highScores) do
        gfx.Save()
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
        gfx.BeginPath()
        local ypos =  50 + (i - 1) * 80
        gfx.Translate(510 + s.xoff, ypos)
        gfx.RoundedRectVarying(0, 0, 280, 70,0,0,35,0)
        gfx.FillColor(15,15,15)
        gfx.StrokeColor(s.color[1], s.color[2], s.color[3])
        gfx.Fill()
        gfx.Stroke()
        gfx.BeginPath()
        gfx.FillColor(255,255,255)
        gfx.FontSize(25)
        gfx.Text(string.format("#%d",i), 5, 5)
        gfx.LoadSkinFont("NovaMono.ttf")
        gfx.FontSize(60)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_TOP)
        gfx.Text(s.score, 140, -4)
        gfx.LoadSkinFont("NotoSans-Regular.ttf")
        gfx.FontSize(20)
        gfx.Text(s.subtext, 140, 45)
        gfx.Restore()
    end
end

draw_gauge_graph = function(x, y, w, h)
    gfx.BeginPath()
    gfx.MoveTo(x, y + h - h * result.gaugeSamples[1])
    
    for i = 2, #result.gaugeSamples do
        gfx.LineTo(x + i * w / #result.gaugeSamples,y + h - h * result.gaugeSamples[i])
    end
    
    gfx.StrokeWidth(2.0)
    if result.flags & 1 ~= 0 then
        gfx.StrokeColor(255,80,0,160)
        gfx.Stroke()
    else
        gfx.StrokeColor(0,180,255,160)
        gfx.Scissor(x, y + h * 0.3, w, h * 0.7)
        gfx.Stroke()
        gfx.ResetScissor()
        gfx.Scissor(x,y-10,w,10+h*0.3)
        gfx.StrokeColor(255,0,255,160)
        gfx.Stroke()
        gfx.ResetScissor()
    end
end

draw_hit_graph_lines = function(x, y, w, h)
    local maxDispDelta = h/2 / hitDeltaScale
    
    gfx.StrokeWidth(1)
    
    for i = -math.floor(maxDispDelta / 10), math.floor(maxDispDelta / 10) do
        local lineY = y + h/2 + i*10*hitDeltaScale
        
        if i == 0 then
            gfx.StrokeColor(128, 255, 128, 128)
        else
            gfx.StrokeColor(64, 128, 64, 64)
        end
        
        gfx.BeginPath()
        gfx.MoveTo(x, lineY)
        gfx.LineTo(x+w, lineY)
        gfx.Stroke()
    end
end

draw_hit_graph = function(x, y, w, h)
    if not hasHitStat or hitDeltaScale == 0.0 then
        return
    end
    
    draw_hit_graph_lines(x, y, w, h)
    gfx.StrokeWidth(1)
    
    for i = 1, #result.noteHitStats do
        local hitStat = result.noteHitStats[i]
        local hitStatY = h/2 + hitStat.delta * hitDeltaScale
        if hitStatY < 0 then hitStatY = 0
        elseif hitStatY > h then hitStatY = h
        end
        if hitStat.rating == 2 then
            gfx.BeginPath()
            gfx.Circle(x+hitStat.timeFrac*w, y+hitStatY, 0.25)
            gfx.StrokeColor(255, 150, 0, 160)
            gfx.Stroke()
        elseif hitStat.rating == 1 then
            gfx.BeginPath()
            gfx.Circle(x+hitStat.timeFrac*w, y+hitStatY, 0.5)
            gfx.StrokeColor(255, 0, 200, 160)
            gfx.Stroke()
        elseif hitStat.rating == 0 then
            gfx.BeginPath()
            gfx.Circle(x+hitStat.timeFrac*w, y+hitStatY, 0.75)
            gfx.StrokeColor(255, 0, 0, 160)
            gfx.Stroke()
        end
    end
end

draw_left_graph = function(x, y, w, h)
    gfx.BeginPath()
    gfx.Rect(x, y, w, h)
    gfx.FillColor(255, 255, 255, 32)
    gfx.Fill()
    
    draw_hit_graph(x, y, w, h)
    draw_gauge_graph(x, y, w, h)
    
    gfx.FontSize(16)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_BOTTOM)
    gfx.BeginPath()
    gfx.FillColor(255, 255, 255, 128)
    gfx.Text(string.format("Mean absolute delta: %.1fms", result.meanHitDeltaAbs), x+4, y+h)
    gfx.Fill()
    
    local endGauge = result.gauge
    local endGaugeY = y + h - h * endGauge
    
    if endGaugeY > y+h - 10 then endGaugeY = y+h - 10
    elseif endGaugeY < y + 10 then endGaugeY = y + 10
    end
    
    local gaugeText = string.format("%.1f%%", endGauge*100)
    
    gfx.FontSize(20)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_MIDDLE)
    local x1, y1, x2, y2 = gfx.TextBounds(x+w-6, endGaugeY, gaugeText)
    
    gfx.BeginPath()
    gfx.FillColor(80, 80, 80, 128)
    gfx.RoundedRect(x1-3, y1, x2-x1+6, y2-y1, 4)
    gfx.Fill()
    
    gfx.BeginPath()
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.FillColor(255, 255, 255)
    gfx.Text(gaugeText, x+w-6, endGaugeY)
end

draw_hit_histogram = function(x, y, w, h)
    if not hasHitStat or hitDeltaScale == 0.0 then
        return
    end
    
    local maxDispDelta = math.floor(h/2 / hitDeltaScale)
    
    local mode = 0
    local modeCount = 0
    
    for i = -maxDispDelta-1, maxDispDelta+1 do
        if hitHistogram[i] == nil then hitHistogram[i] = 0 end
    end

    for i = -maxDispDelta, maxDispDelta do
        local count = hitHistogram[i-1] + hitHistogram[i]*2 + hitHistogram[i+1]
        
        if count > modeCount then
            mode = i
            modeCount = count
        end
    end
    
    gfx.StrokeWidth(1.5)
    gfx.BeginPath()
    gfx.StrokeColor(255, 255, 128, 96)
    gfx.MoveTo(x, y)
    for i = -maxDispDelta, maxDispDelta do
        local count = hitHistogram[i-1] + hitHistogram[i]*2 + hitHistogram[i+1]
        
        gfx.LineTo(x + 0.8 * w * count / modeCount, y+h/2 + i*hitDeltaScale)
    end
    gfx.LineTo(x, y+h)
    gfx.Stroke()
end

draw_right_graph = function(x, y, w, h)
    if not hasHitStat or hitDeltaScale == 0.0 then
        return
    end
    
    gfx.BeginPath()
    gfx.Rect(x, y, w, h)
    gfx.FillColor(64, 64, 64, 32)
    gfx.Fill()
    
    draw_hit_graph_lines(x, y, w, h)
    draw_hit_histogram(x, y, w, h)
    
    local meanY = h/2 + hitDeltaScale * result.meanHitDelta
    local medianY = h/2 + hitDeltaScale * result.medianHitDelta
    
    drawLine(x, y+meanY, x+w, y+meanY, 1.25, 255, 0, 0, 192)
    drawLine(x, y+medianY, x+w, y+medianY, 1.25, 64, 64, 255, 192)
    
    gfx.LoadSkinFont("NovaMono.ttf")
    
    gfx.BeginPath()
    if meanY < medianY then
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_BOTTOM)
    else
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
    end
    gfx.FillColor(255, 128, 128)
    gfx.FontSize(16)
    gfx.Text(string.format("Mean: %.1f ms", result.meanHitDelta), x+5, y+meanY)
    
    gfx.BeginPath()
    if medianY <= meanY then
        gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_BOTTOM)
    else
        gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_TOP)
    end
    gfx.FillColor(196, 196, 255)
    gfx.FontSize(16)
    gfx.Text(string.format("Median: %d ms", result.medianHitDelta), x+w-5, y+medianY)
    
    gfx.FillColor(255, 255, 255)
    gfx.FontSize(15)
    
    gfx.BeginPath()
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
    gfx.Text(string.format("Earliest: %d ms", hitMinDelta), x+5, y)
    
    gfx.BeginPath()
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_BOTTOM)
    gfx.Text(string.format("Latest: %d ms", hitMaxDelta), x+5, y+h)
end

-- Main components

draw_title = function(x, y, w, h)
    local centerLineY = y+h*0.6
    
    gfx.LoadSkinFont("NotoSans-Regular.ttf")
    
    gfx.BeginPath()
    gfx.FillColor(255, 255, 255)
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
    
    gfx.FontSize(48)
    drawScaledText(result.title, x+w/2, centerLineY-18, w/2-5)
    
    drawLine(x+30, centerLineY, x+w-30,centerLineY, 1, 64, 64, 64)
    
    gfx.FontSize(27)
    drawScaledText(result.artist, x+w/2, centerLineY+28, w/2-5)
end

draw_chart_info = function(x, y, w, h, full)
    local jacket_size = 250
    
    local jacket_y = y+40
    
    if not full then
        jacket_y = y
        jacket_size = 300
    end
    
    local jacket_x = x+(w-jacket_size)/2
    
    gfx.LoadSkinFont("NotoSans-Regular.ttf")
    
    gfx.BeginPath()
    if jacketImg ~= nil then
        gfx.ImageRect(jacket_x, jacket_y, jacket_size, jacket_size, jacketImg, 1, 0)
    else
        gfx.BeginPath()
        gfx.FillColor(0, 0, 0, 128)
        gfx.Rect(jacket_x, jacket_y, jacket_size, jacket_size)
        gfx.Fill()
        
        gfx.BeginPath()
        gfx.FillColor(255, 255, 255, 160)
        gfx.FontSize(30)
        gfx.Text("No Image", x+w/2, y+jacket_size/2+60)
    end
    
    if full then
        gfx.BeginPath()
        gfx.FillColor(255, 255, 255)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
        gfx.FontSize(30)
        gfx.Text(string.format("%s %02d", diffNames[result.difficulty + 1], result.level), x+w/2, y+30)
    else
        local level_text = string.format("%s %02d", diffNames[result.difficulty + 1], result.level)
        if result.effector ~= nil and result.effector ~= "" then
            level_text = string.format("%s, by %s", level_text, result.effector)
        end
        
        gfx.FontSize(20)
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_TOP)
        local x1, y1, x2, y2 = gfx.TextBounds(0, 0, level_text)
        local box_width = x2+10
        if box_width > jacket_size then box_width = jacket_size end
        
        gfx.BeginPath()
        gfx.FillColor(0, 0, 0, 200)
        gfx.RoundedRectVarying(jacket_x, jacket_y, box_width, 25, 0, 0, 5, 0)
        gfx.Fill()
        
        gfx.FillColor(255, 255, 255)
        drawScaledText(level_text, jacket_x+5, jacket_y+2, jacket_size-10)
    
        local graph_height = jacket_size * 0.3
        local graph_y = jacket_y+jacket_size - graph_height
    
        gfx.BeginPath()
        gfx.FillColor(0,0,0,200)
        gfx.Rect(jacket_x, graph_y, jacket_size, graph_height)
        gfx.Fill()
        draw_gauge_graph(jacket_x, graph_y, jacket_size, graph_height)
        
        if gradeImg ~= nil then
            gfx.BeginPath()
            gfx.ImageRect(jacket_x+jacket_size-60*gradeAR, jacket_y+jacket_size-60, 60*gradeAR, 60, gradeImg, 1, 0)
        end
        
        gfx.BeginPath()
        gfx.FillColor(255,255,255)
        gfx.FontSize(20)
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_MIDDLE)
        gfx.Text(string.format("%.1f%%", result.gauge*100), jacket_x+jacket_size+10, jacket_y+jacket_size-graph_height*result.gauge)
        return
    end
    
    draw_y = jacket_y + jacket_size + 27
    
    if result.effector ~= nil and result.effector ~= "" then
        gfx.FillColor(255, 255, 255)
        gfx.FontSize(16)
        gfx.Text("Effected by", x+w/2, draw_y)
        gfx.FontSize(27)
        drawScaledText(result.effector, x+w/2, draw_y+24, w/2-5)
        draw_y = draw_y + 50
    end
    
    if result.illustrator ~= nil and result.illustrator ~= "" then
        gfx.FontSize(16)
        gfx.Text("Illustrated by", x+w/2, draw_y)
        gfx.FontSize(27)
        drawScaledText(result.illustrator, x+w/2, draw_y+24, w/2-5)
        draw_y = draw_y + 50
    end
end

draw_basic_hitstat = function(x, y, w, h, full)    
    local grade_width = 70 * gradeAR
    local stat_y = y+12
    local stat_gap = 15
    local stat_size = 30
    local stat_width = w-8
    
    if full then
        stat_gap = 6
        stat_size = 25
        stat_width = w-18
        
        if result.retryCount == nil or result.retryCount <= 0 then
            stat_gap = 25
            stat_y = stat_y + 5
        end
        
        gfx.BeginPath()
        gfx.ImageRect(x + (w-grade_width)/2 - 5, stat_y, grade_width, 70, gradeImg, 1, 0)
        stat_y = stat_y + 85
    else
        if result.retryCount == nil or result.retryCount <= 0 then
            stat_gap = 30
        end
    end
    
    
    if clearText ~= "" then
        gfx.BeginPath()
        gfx.FillColor(255, 255, 255)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
        gfx.FontSize(20)
        gfx.Text(clearText, x+w/2 - 5, stat_y)
    end
    
    stat_y = stat_y + 50
    draw_score(result.score, x, stat_y, w, 72)
    
    stat_y = stat_y + 19
    
    if highestScore > 0 then
        if highestScore > result.score then
            gfx.FillColor(255, 0, 0)
            draw_score(highestScore - result.score, x+w/2, stat_y, w/2, 25, "-")
        elseif highestScore == result.score then
            gfx.FillColor(128, 128, 128)
            draw_score(0, x+w/2, stat_y, w/2, 25, utf8.char(0xB1))
        else
            gfx.FillColor(0, 255, 0)
            draw_score(result.score - highestScore, x+w/2, stat_y, w/2, 25, "+")
        end
    end
    
    stat_y = stat_y + stat_gap
    
    gfx.FillColor(255, 255, 255)
    
    stat_y = draw_stat(x+4, stat_y, stat_width, stat_size, "CRIT", result.perfects, "%d", 255, 150, 0)
    stat_y = draw_stat(x+4, stat_y, stat_width, stat_size, "NEAR", result.goods, "%d", 255, 0, 200)
    
    local early_late_width = w/2-20
    local late_x = x+stat_width-early_late_width
    draw_stat(late_x-early_late_width-10, stat_y, early_late_width, stat_size-6, "EARLY", result.earlies, "%d", 255, 0, 255)
    draw_stat(late_x, stat_y, early_late_width, stat_size-6, "LATE", result.lates, "%d", 0, 255, 255)
    
    stat_y = stat_y + stat_size + 5
    stat_y = draw_stat(x+4, stat_y, stat_width, stat_size, "ERROR", result.misses, "%d", 255, 0, 0)
    
    stat_y = draw_stat(x+4, stat_y+15, stat_width, stat_size, "MAX COMBO", result.maxCombo, "%d", 255, 255, 0)
    
    if result.retryCount ~= nil and result.retryCount > 0 then
        stat_y = draw_stat(x+4, stat_y+15, stat_width, stat_size-6, "RETRY", result.retryCount, "%d")
        
        if result.mission ~= nil and result.mission ~= "" then
            gfx.LoadSkinFont("NotoSans-Regular.ttf")
            gfx.TextAlign(gfx.TEXT_ALIGN_TOP + gfx.TEXT_ALIGN_LEFT)
            
            gfx.BeginPath()
            gfx.FontSize(16)
            gfx.Text(string.format("Mission: %s", result.mission), x+4, stat_y)
        end
    end
end

draw_graphs = function(x, y, w, h)    
    if not hasHitStat or hitDeltaScale == 0.0 then
        draw_left_graph(x, y, w, h)
    else
        draw_left_graph(x, y, w - w//4, h)
        draw_right_graph(x + (w - w//4), y, w//4, h)
    end
end

draw_footer = function(x, y, w, h)
    gfx.LoadSkinFont("NotoSans-Regular.ttf")
    
    if result.playbackSpeed ~= nil and result.playbackSpeed ~= 1.00 then
        gfx.FontSize(30)
        gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_MIDDLE)
        
        if result.playbackSpeed < 1.00 then
            gfx.FillColor(128, 255, 128)
        else
            gfx.FillColor(255, 128, 64)
        end
        
        gfx.BeginPath()
        gfx.Text(string.format("x%.2f play", result.playbackSpeed), x+w-10, y+h/2)
    end
end

render = function(deltaTime, showStats)
    local resx,resy = game.GetResolution()
    
    if resx ~= currResX or resy ~= currResY then
        scale = math.min(resx / desw, resy / desh)
        
        if resx / resy > 1 then
            moveX = resx / (2*scale) - desw / 2
        else
            moveY = resy / (2*scale) - desh / 2
        end
        
        currResX = resX
        currResY = resY
    end
    
    if prevShowStats ~= showStats then
        prevShowStats = showStats
        
        if showStats then
            showStatsHit = not showStatsHit
        end
    end
    
    -- Background image
    gfx.BeginPath()
    gfx.ImageRect(0, 0, resx, resy, backgroundImage, 0.5, 0);
    gfx.Scale(scale,scale)
    gfx.Translate(moveX,moveY)
    
    gfx.BeginPath()
    gfx.Rect(0,0,500,800)
    gfx.FillColor(30,30,30,128)
    gfx.Fill()
    
    -- Result
    draw_title(0, 0, 500, 110)
    if showStatsHit then
        draw_chart_info(0, 120, 280, 420, true)
        draw_basic_hitstat(280, 120, 220, 420, true)
        draw_graphs(0, 540, 500, 200)
    else
        draw_chart_info(0, 120, 500, 310, false)
        draw_basic_hitstat(50, 430, 400, 400, false)
    end
    draw_footer(0, 750, 500, 50)
    
    -- Highscores
    draw_highscores()
    
    -- Applause SFX
    if result.badge > 1 and not played then
        game.PlaySample("applause")
        played = true
    end
    
    -- Screenshot notification
    shotTimer = math.max(shotTimer - deltaTime, 0)
    if shotTimer > 1 then
        draw_shotnotif(505,755);
    end
end

get_capture_rect = function()
    local x = moveX * scale
    local y = moveY * scale
    local w = 500 * scale
    local h = 800 * scale
    return x,y,w,h
end

screenshot_captured = function(path)
    shotTimer = 10;
    shotPath = path;
    game.PlaySample("shutter")
end
