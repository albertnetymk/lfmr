load ./db_queue.mat;
% queue of mr algo
ebr=1; rc=2; hp=3; dw=4; jvm=5;
gcs = [ebr rc hp dw jvm];
threads = [1 2 4 8 16 31 32 63 64];
elements = [100 200 400 800 1600];

exclude_rc = 0;
mylegend = {"ebr"; "rc"; "hp"; "encore"; "jvm"};

if exclude_rc == 1
    mylegend(2,:) = [];
end
directory_prefix = 'plots/queue'
function myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
    close all
    fullscreen = get(0,'ScreenSize');
    figure(1, 'Position',[0 -50 fullscreen(3) fullscreen(4)]);
    clf;
    y = y/1000;
    err = err/1000;
    if size(x, 2) == 4
        errorbar(...
            x(:, 1), y(:, 1), err(:, 1), '-b', ...
            x(:, 2), y(:, 2), err(:, 2), '-*r', ...
            x(:, 3), y(:, 3), err(:, 3), '-ok', ...
            x(:, 4), y(:, 4), err(:, 4), '-pm' ...
            )
    else
        errorbar(...
            x(:, 1), y(:, 1), err(:, 1), '-b', ...
            x(:, 2), y(:, 2), err(:, 2), '-sg', ...
            x(:, 3), y(:, 3), err(:, 3), '-*r', ...
            x(:, 4), y(:, 4), err(:, 4), '-ok', ...
            x(:, 5), y(:, 5), err(:, 5), '-pm' ...
            )
    end
    title(mytitle, 'fontsize', 15)
    xlabel(myxlabel, 'fontsize', 15)
    ylabel(myylabel, 'fontsize', 15)
    yl = ylim;
    axis([x(1), x(end), 0, yl(end)])
    set(gca, 'xtick', xtick)
    set(gca, 'fontsize', 15)
    lh = legend(mylegend);
    % set(lh,'FontSize',15);
    print(1, filename)
end

data_avg = 0;
data_std = 0;
for i = 1:2
    if i == 1
        prefix = strcat(directory_prefix, '/exec_time');
        data_avg = db_queue_total_avg;
        data_std = db_queue_total_std;
        myylabel = 'cpu time per operation (microseconds)';
    else
        prefix = strcat(directory_prefix, '/footprint');
        data_avg = db_queue_footprint_avg;
        data_std = db_queue_footprint_std;
        myylabel = 'footprint (kilybytes)';
    end

    for t = threads
        xtick = elements;
        x = repmat(xtick', 1, size(gcs, 2));
        y = x;
        err = x;
        for gc = gcs
            tmp = data_avg(gc, find(t == threads), :);
            tmp = squeeze(tmp);
            y(:, gc) = tmp;
            tmp = data_std(gc, find(t == threads), :);
            tmp = squeeze(tmp);
            err(:, gc) = tmp;
        end
        if exclude_rc == 1
            x(:, 2) = [];
            y(:, 2) = [];
            err(:, 2) = [];
        end
        mytitle = ...
          sprintf("lockfree queue #threads=%d", t);
        myxlabel = '#elements';
        filename = sprintf("%s/queue_%d_%s.png", prefix, t, 'x');
        myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
    end

    for e = elements
        xtick = threads;
        x = repmat(xtick', 1, size(gcs, 2));
        y = x;
        err = x;
        for gc = gcs
            tmp = data_avg(gc, :, find(e == elements));
            tmp = squeeze(tmp);
            y(:, gc) = tmp;
            tmp = data_std(gc, :, find(e == elements));
            tmp = squeeze(tmp);
            err(:, gc) = tmp;
        end
        if exclude_rc == 1
            x(:, 2) = [];
            y(:, 2) = [];
            err(:, 2) = [];
        end
        mytitle = ...
          sprintf("lockfree queue #elements=%d", e);
        myxlabel = '#threads';
        filename = sprintf("%s/queue_%s_%d.png", prefix, 'x', e);
        myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
    end
end
return
