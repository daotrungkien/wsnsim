trial = 1;


file = fopen(sprintf('..\\Debug\\wsnsim-log-test1-trial-%d.txt', trial));
tdata = textscan(file, '%f %s %f');
fclose(file);


fig = figure;
hold on

lmax = 4000;
tmax = 180;
sunrise = 5 + 15/60;
sunset = 18 + 33/60;
nodes = {'temperature', 'humidity', 'light'};
colors = {'red', 'green', 'blue'};
max_levels = [3500, 4000, 2500];
charge_level = .2;

t = -24;
while t < tmax
    rectangle('Position', [t+sunset, 0, 24+sunrise-sunset, lmax], 'FaceColor', [.9 .9 .9], 'EdgeColor', 'none');
    t = t + 24;
end


for n = 1:3
    t = [];
    v = [];
    for i = 1 : length(tdata{1})
        if strcmp(tdata{2}{i}, nodes{n})
            t = [t tdata{1}(i)];
            v = [v tdata{3}(i)];
        end
    end
    
    plot(t/3600, v, 'Color', colors{n}, 'LineWidth', 2);
end

legend(nodes);
legend('show');
xlim([0 tmax]);
ylim([0 lmax]);
xlabel('Time (hours)');
ylabel('Capacity (mAh)');


for n = 1:3
    cl = charge_level*max_levels(n);
    plot([0 tmax], [cl cl], 'Color', colors{n}, 'LineWidth', 1, 'LineStyle', '--');
end
