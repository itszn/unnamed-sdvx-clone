background.LoadTexture("mainTex", "bg.png")
background.SetSpeedMult(0.3)

function render_bg(deltaTime)
  background.DrawShader()
end
