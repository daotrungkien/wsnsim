trial = 1;


file = fopen('..\\Debug\\wsnsim-log-test2-nodes.txt');
ndata = textscan(file, '%f %s %f %f %f %s');
fclose(file);

file = fopen(sprintf('..\\Debug\\wsnsim-log-test2-trial-%d.txt', trial));
cdata = textscan(file, '%f %s %s %s');
fclose(file);


fig = figure;
hold on
xlim([-5 35])
ylim([-5 35])
daspect([1 1 1])


node_idx = @(name)  find(strcmp(ndata{2}, name));
circle = @(x, y, r, color)  rectangle('Position', [x-r,y-r,2*r,2*r], 'Curvature', [1,1], 'EdgeColor', color);
fcircle = @(x, y, r, color) rectangle('Position', [x-r,y-r,2*r,2*r], 'Curvature', [1,1], 'FaceColor', color);

node_size = .4;
node_ring = .6;
master_node = 'temp1';
tmin = min(cdata{1});
tmax = max(cdata{1});

ndata{7}(1:length(ndata{1})) = -1;

for i = 1 : length(cdata{1})
    t = cdata{1}(i);
    name = cdata{2}{i};
    from = cdata{3}{i};
    action = cdata{4}{i};

    idx = node_idx(name);
    x = ndata{3}(idx);
    y = ndata{4}(idx);
    
    if ~strcmp(from, '-')
        from_idx = node_idx(from);
        fx = ndata{3}(from_idx);
        fy = ndata{4}(from_idx);
        plot([x fx], [y fy], 'Color', [.8 .8 .8]);
    end
    
    switch action
        case 'measure'
            circle(x, y, node_ring, 'blue');
        case 'drop'
            circle(x, y, node_ring, 'red');
    end
    
    ndata{7}(idx) = t - tmin;
end


for i = 1 : length(ndata{1})
    name = ndata{2}{i};
    x = ndata{3}(i);
    y = ndata{4}(i);
    t = ndata{7}(i);
    
    if strcmp(name, master_node)
        circle(x, y, node_ring, 'green');
    end
    
    if t < 0
        fcircle(x, y, node_size, 'red')
    else
        fcircle(x, y, node_size, [0 0 t/(tmax-tmin)])
    end
    
    text(x, y-1.5, name, 'HorizontalAlignment', 'center', 'Color', [.5 .5 .5])
end
