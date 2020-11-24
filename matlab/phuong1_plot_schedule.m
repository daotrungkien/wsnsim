chromosome = "0+ 10.0015 24.0011+ 61.3609 64.0242+ 81.6472 95.4332+ 147.525 175.989+ 203.403 205.992+ 226.942 237.396+ 253.751 310.793+ 327.068 416.823+ 763.84 1440+";
arr = split(chromosome);
arr = arr(strlength(arr) > 0);

xa = [];
ya = [];
for i = 1 : length(arr)
    ai = arr{i};
    
    if ai(end) == '+'
        y_next = 1;
        x_next = str2double(ai(1 : end-1));
    else
        y_next = 0;
        x_next = str2double(ai);
    end
    
    if i > 1
        xa = [xa x x_next];
        ya = [ya y y];
    end
    
    y = y_next;
    x = x_next;
end

plot(xa / 60, ya, 'LineWidth', 2);
box on
grid on
xlabel('Time (s)');
ylabel('On/off schedule');
xlim([0 1440] / 60)
ylim([-0.5 1.5])
