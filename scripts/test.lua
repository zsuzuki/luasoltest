-- Hello
-- test lua file

local App = {}

function App:init()
    cnt = 0
    msg = "Hello"
end

function App:update()
    print(cnt,msg)
    cnt = cnt + 1
end

local t = Test.new("OK")
t:print()
App.init()
for i = 0,10
do
    App.update()
end
