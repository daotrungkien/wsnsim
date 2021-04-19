trial = 1;
path = '..\\Release\\';

lmax = 40000;
tmax = 70;


file = fopen(sprintf([path 'wsnsim-log-phuong1-trial-%d.txt'], trial));
tdata = textscan(file, '%f %s %f');
fclose(file);

%%

global nodes colors
fig = createplot(tmax, lmax)


t = {};
v = {};
for n = 1:3
    t{n} = [];
    v{n} = [];
    for i = 1 : length(tdata{1})
        if strcmp(tdata{2}{i}, nodes{n})
            t{n}(end+1) = tdata{1}(i);
            v{n}(end+1) = tdata{3}(i);
        end
    end
    
    plot(t{n}/3600, v{n}, 'Color', colors{n}, 'LineWidth', 2, 'DisplayName', nodes{n});
end

vmax = max([max(v{1}) max(v{2}) max(v{3})]);

legend('show');
xlim([0 tmax]);
ylim([0 vmax]);
xlabel('Time (hours)');
ylabel('Capacity (mAh)');
box on

%% energy harvesting

te = {};
e = {};

fig = createplot(tmax, lmax)

for n = 1:3
    te{n} = [];
    e{n} = [];
    
    for i = 1 : length(tdata{1})
        if strcmp(tdata{2}{i}, nodes{n})
            if isempty(e{n})
                te{n}(end+1) = 0;
                e{n}(end+1) = 0;
            else
                if tdata{3}(i) > last
                    te{n}(end+1) = tdata{1}(i);
                    e{n}(end+1) = e{n}(end) + tdata{3}(i) - last;
                end
            end
            
            last = tdata{3}(i);
        end
    end
    
    plot(te{n}/3600, e{n}, 'Color', colors{n}, 'LineWidth', 2, 'DisplayName', nodes{n});
end

emax = max([max(e{1}) max(e{2}) max(e{3})]);

legend('show');
xlim([0 tmax]);
ylim([0 emax]);
xlabel('Time (hours)');
ylabel('Harvested energy (mAh)');
box on


%% createplot

function fig = createplot(tmax, lmax)
    global nodes colors max_levels

    fig = figure;
    hold on

    sunrise = 5 + 15/60;
    sunset = 18 + 33/60;
    nodes = {'S1', 'S2', 'S3'};
    colors = {'red', 'green', 'blue'};
    max_levels = [3500, 4000, 2500];

    t = -24;
    while t < tmax
        rectangle('Position', [t+sunset, 0, 24+sunrise-sunset, lmax], 'FaceColor', [.9 .9 .9], 'EdgeColor', 'none');
        t = t + 24;
    end
end